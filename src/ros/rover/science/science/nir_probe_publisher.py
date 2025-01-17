#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 receiver code for the
nir probe (v2) publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: nir_probe_publisher
TOPICS:
  - /science/nir_probe_data [NIRProbeData]
    [Published]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     science
AUTHOR(S):   Brandon Chung
CREATION:    14/01/2025
EDITED:      17/01/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This package bridges turning on LEDs with
collecting NIR photodiode data.

The only commands to send this publisher are:
    0. Stop reading from photodiodes
    1. Read from photodiode 1
    2. Read from photodiode 2
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import logging
from statistics import fmean, stdev
import rclpy, jcan
from rclpy.node import Node

# import custom messages
from nova_interfaces.msg import NIRProbeData
from nova_interfaces.srv import SetNIRProbeLED

class NIRProbePublisher(Node):

    # Selecting CAN
    CAN_BUS = "can1"
    CAN_BUS_PARAM = "can_bus"

    # CARD IDs
    NIR_PROBE_ID = 0x0F0

    NIR_RECEIVE_ID = [
        PHOTODIODE1_ID := 0x4F0,
        PHOTODIODE2_ID := 0x4F1,
    ]

    # Commands
    LED_COMMANDS = [
        TURN_LED1_ON := 0x01,
        TURN_LED2_ON := 0x02,
        TURN_LED_OFF := 0x03,
    ]

    PHOTODIODE_COMMANDS = [
        READ_PHOTODIODE1 := 0x04,
        READ_PHOTODIODE2 := 0x05,
    ]

    # All active photodiode states 
    PHTODIODE_STATES = [
        PHOTODIODE_OFF := (0).to_bytes(1, "big"), # Turns off both photodiodes
        PHOTODIODE1_ON := (1).to_bytes(1, "big"), # Exclusively turns on photodiode 1
        PHOTODIODE2_ON := (2).to_bytes(1, "big"), # Exclusively turns on photodiode 2
    ]

    # Timings
    SEND_INTERVAL                 = 0.01
    READ_INTERVAL                 = SEND_INTERVAL

    # Number of readings before moving on
    PHOTODIODE_LIGHT_BLANK_PERIOD = READ_INTERVAL * 100
    PHOTODIODE_CALIBRATION_PERIOD = READ_INTERVAL * 100
    PHOTODIODE_ONLINE_PERIOD      = READ_INTERVAL * 30

    # Stages of photodiode reading
    STAGES = [
        STAGE_LIGHT_BLANK := 0,
        STAGE_CALIBRATION := 1,
        STAGE_ONLINE      := 2,
    ]

    def __init__(self):

        # Initialise publisher
        super().__init__('nir_probe_publisher')
        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("NIR Probe Publisher starting")
        self.publisher = self.create_publisher(NIRProbeData, '/science/nir_probe_data', 10)

        # Initialise CAN bus
        self.declare_parameter(self.CAN_BUS_PARAM, self.CAN_BUS)
        self.bus = jcan.Bus()

        # Only accept inputs being delivered to receiving CAN IDs
        self.bus.set_id_filter(self.NIR_RECEIVE_ID)
        self.bus.add_callback(self.PHOTODIODE1_ID, self.read_data_callback)
        self.bus.add_callback(self.PHOTODIODE2_ID, self.read_data_callback)

        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)

        # Initialise service for taking commands
        self.command_service = self.create_service(SetNIRProbeLED, '/science/set_nir_probe_led', self.command_service_callback)

        # Initialise timers
        self.photodiode_timers = [
            self.photodiode_data_timer,
            self.photodiode_light_blank_timer,
            self.photodiode_calibration_timer,
            self.photodiode_online_timer,
        ] = [
            self.create_timer(self.READ_INTERVAL, self.send_read_command_callback),
            self.create_timer(self.PHOTODIODE_LIGHT_BLANK_PERIOD, self.photodiode_light_blank_callback),
            self.create_timer(self.PHOTODIODE_CALIBRATION_PERIOD, self.photodiode_calibration_callback),
            self.create_timer(self.PHOTODIODE_ONLINE_PERIOD, self.photodiode_online_callback),
        ]
        
        self.timer_jcan_spin = self.create_timer(self.SEND_INTERVAL, self.bus.spin)

        self.reset_variables()

        self.get_logger().info(f"NIR Probe Publisher started on {self.get_parameter(self.CAN_BUS_PARAM).value}") 


    """
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Support Functions
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """
    def reset_variables(self):
        """
        Function to disable all photodiode timers
        Good for turning off photodiode
        """
        self.led_on = False
        self.active_photodiode = self.PHOTODIODE_OFF
        self.stage = self.STAGE_LIGHT_BLANK
        self.data = [[] for _ in range(len(self.STAGES))]
        self.average_data = [0.0] * len(self.STAGES)
        self.standard_deviation = [0.0] * len(self.STAGES)
        for timer in self.photodiode_timers:
            timer.cancel() 


    def publish_msg(self, data: float, std_dev: float, led_on: bool):
        """
        Publish data to GUI
        """
        msg = NIRProbeData()
        msg.data = data
        msg.std_dev = std_dev
        msg.active_photodiode = self.active_photodiode
        msg.led_on = led_on
        self.get_logger().debug(f"Publishing {msg}")
        self.publisher.publish(msg)


    """
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ROS2 Functions
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """

    def send_read_command_callback(self):
        """
        Sends the read data command to the NIR probe
        """
        command = self.TURN_LED_OFF

        match self.active_photodiode:
            case self.PHOTODIODE1_ON:
                command = self.READ_PHOTODIODE1
            case self.PHOTODIODE2_ON:
                command = self.READ_PHOTODIODE2
            case self.PHOTODIODE_OFF:
                return # Do not send command to read photodiode if they should be off
            case _:
                self.get_logger().error(f"Invalid LED request received by read data callback: {self.command}")

        try:
            self.bus.send(jcan.Frame(self.NIR_PROBE_ID, [command]))
            self.get_logger().debug(f"Sent read command over CAN for photodiode {self.active_photodiode}")

        except Exception as e:
            print(e)
            self.get_logger().error(f"Failed to send read command over CAN for photodiode {self.active_photodiode}")


    def read_data_callback(self, frame: jcan.Frame):
        """
        Callback for when data is received from the NIR probe
        Save this data to send as one value at the end of online stage
        """
        # Receive data if corresponding LED is on
        if frame.id not in self.NIR_RECEIVE_ID: 
            self.get_logger().warn(f"Received unknown frame {frame}")
            return

        self.get_logger().debug(f"Received {hex(frame.id)} {frame.data}")

        value = int.from_bytes(frame.data, "big")

        self.data[self.stage].append(value)
        self.get_logger().debug(f"Saving {value} for stage {self.stage}")



    def command_service_callback(self, request, response):
        """
        Callback to record data from NIR Probe photodiodes
        """
        response.success = True

        message = int.from_bytes(request.command, "big")

        # Check which led state NIR Probe publisher is in
        match request.command:

            case self.PHOTODIODE_OFF:
                self.get_logger().debug("Turning off LEDs and photodiodes.") 

                # Turn off photodiode timer
                self.reset_variables()

                # Turn LED off
                self.bus.send(jcan.Frame(self.NIR_PROBE_ID, [self.TURN_LED_OFF]))
                return response

            case self.PHOTODIODE1_ON | self.PHOTODIODE2_ON:
                # Turn off photodiode timer
                self.reset_variables()

                # Turn LED on
                self.get_logger().info(f"Turning on NIR probe LED {request.led}")

                # Turn on photodiode timer
                self.photodiode_light_blank_timer.reset()

            case _:
                self.get_logger().error(f"Invalid LED request made: {request.led}")
                response = False
                return response


        self.frame = jcan.Frame(self.NIR_PROBE_ID, [message]) # Save frame to turn on light at the end of light blanking

        self.command = request.command

        self.get_logger().debug(f"Activating photodiode {request.command}")

        self.publish_msg(-1, -1, False) # -1 to not save these values on the GUI side

        return response


    def photodiode_light_blank_callback(self):
        """
        One-shot callback that deactivates light blanking stage after period is over and activate calibration stage
        """
        try:
            self.get_logger().debug(f"Sending {self.frame}")
            self.bus.send(self.frame) # Turn lights on

        except Exception as e:
            self.get_logger().error(f"Failed to send led command over CAN: {e}")

        self.photodiode_light_blank_timer.cancel()
        self.stage += 1

        self.average_data[0] = fmean(self.data[0])
        self.standard_deviation[0] = stdev(self.data[0])

        self.get_logger().debug(f"Entering calibration mode\nSending light blank average to gui: {self.average_data[0]}")

        self.publish_msg(self.average_data[0], self.standard_deviation[0], True)

        self.photodiode_calibration_timer.reset()


    def photodiode_calibration_callback(self):
        """
        One-shot callback that deactivates calibration stage after period is over and activates online stage
        """
        self.photodiode_calibration_timer.cancel()
        self.stage += 1

        self.average_data[1] = fmean(self.data[1])
        self.standard_deviation[1] = stdev(self.data[1])

        self.get_logger().debug(f"Entering online mode\nSending calibration mode average to gui: {self.average_data[1]}")

        self.publish_msg(self.average_data[1], self.standard_deviation[1], True)

        self.photodiode_online_timer.reset()


    def photodiode_online_callback(self):
        """
        One-shot callback that deactivates photodiode after period is over
        """
        self.average_data[2] = fmean(self.data[2])
        self.standard_deviation[2] = stdev(self.data[2])

        # Calculate light difference
        light_difference = self.average_data[2] - self.average_data[0]

        self.get_logger().debug(f"Turning off photodiode and LED\nSending light difference to gui: {light_difference}")

        self.publish_msg(light_difference, self.standard_deviation[2], True)

        self.reset_variables()


# The main code that executes when starting
def main(args=None):

    # Create the publisher
    rclpy.init(args = args)
    publisher = NIRProbePublisher()
    rclpy.spin(publisher)

    #  Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__=="__main__":
    main()
