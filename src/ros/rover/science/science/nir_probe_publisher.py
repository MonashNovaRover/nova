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
EDITED:      15/01/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This package bridges turning on LEDs with
collecting NIR photodiode data.

The only commands to send this publisher are:
    1. Turn on LED 1
    2. Turn on LED 2
    3. Turn off LEDs
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

    # All states NIR Probe may be in
    LEDS = [
        LED_OFF := (0).to_bytes(1, "big"), # Turns off both LEDs
        LED1_ON := (1).to_bytes(1, "big"), # Turns on LED 1, Turns off LED 2
        LED2_ON := (2).to_bytes(1, "big"), # Turns on LED 2, Turns off LED 1
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
        STAGE_OFFLINE     := 0,
        STAGE_LIGHT_BLANK := 1,
        STAGE_CALIBRATION := 2,
        STAGE_ONLINE      := 3,
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
        self.led_service = self.create_service(SetNIRProbeLED, '/science/set_nir_probe_led', self.led_service_callback)

        # Initialise timers
        self.photodiode_timers = [
            self.photodiode_data_timer,
            self.photodiode_light_blank_timer,
            self.photodiode_calibration_timer,
            self.photodiode_online_timer,
        ] = [
            self.create_timer(self.READ_INTERVAL, self.send_read_data_callback),
            self.create_timer(self.PHOTODIODE_LIGHT_BLANK_PERIOD, self.light_blank_callback),
            self.create_timer(self.PHOTODIODE_CALIBRATION_PERIOD, self.calibration_callback),
            self.create_timer(self.PHOTODIODE_ONLINE_PERIOD, self.online_callback),
        ]
        for timer in self.photodiode_timers:
            timer.cancel()


        # Variables
        self.photodiode_on = False
        self.led_on = False
        self.led = self.LED_OFF
        self.stage = self.STAGE_OFFLINE
        self.data = [[] for _ in range(3)]
        self.average_data = [0.0, 0.0, 0.0]
        self.standard_deviation = [0.0, 0.0, 0.0]

        self.timer_jcan_spin = self.create_timer(self.SEND_INTERVAL, self.bus.spin)

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
        self.photodiode_on = False
        self.led_on = False
        self.led = self.LED_OFF
        self.stage = self.STAGE_OFFLINE
        self.data = [[] for _ in range(3)]
        self.average_data = [0.0, 0.0, 0.0]
        self.standard_deviation = [0.0, 0.0, 0.0]
        for timer in self.photodiode_timers:
            timer.cancel()


    def publish_msg(self, data: float, std_dev: float, led_on: bool, photodiode_on:bool):
        """
        Publish data to GUI
        """
        msg = NIRProbeData()
        msg.data = data
        msg.std_dev = std_dev
        msg.led = self.led
        msg.led_on = led_on
        msg.photodiode_on = photodiode_on
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
        try:
            command = self.TURN_LED_OFF

            match self.led:
                case self.LED1_ON:
                    command = self.READ_PHOTODIODE1
                case self.LED2_ON:
                    command = self.READ_PHOTODIODE2
                case self.LED_OFF:
                    return
                case _:
                    self.get_logger().error(f"Invalid LED request received by read data callback: {self.led}")

            self.bus.send(jcan.Frame(self.NIR_PROBE_ID, [command]))

        except Exception as e:
            print(e)
            self.get_logger().error(f"Failed to send read command over CAN for photodiode {self.led}")


    def read_data_callback(self, frame: jcan.Frame):
        """
        Callback for when data is received from the NIR probe
        Save this data to send as one value at the end of online stage
        """
        # Receive data if corresponding LED is on
        match frame.id:

            case self.PHOTODIODE1_ID | self.PHOTODIODE2_ID:
                self.get_logger().debug(f"Received {hex(frame.id)} {frame.data}")

                value = int.from_bytes(frame.data, "big")

                self.data[self.stage-1].append(value) # Offset for offline stage

                self.get_logger().debug(f"Saving {value} for stage {self.stage}")

            case _:
                self.get_logger().warn(f"Received unknown frame {frame}")


    def led_service_callback(self, request, response):
        """
        Callback to turn the NIR probe LEDs on or off
        """
        response.success = False

        message = int.from_bytes(request.led, "big")

        # Check which led state NIR Probe publisher is in
        match request.led:

            case self.LED_OFF:
                self.get_logger().debug("Turning off LEDs and photodiodes.")

                # Turn off photodiode timer
                self.reset_variables()

            case self.LED_1_ON | self.LED2_ON:
                # Turn LED on
                self.get_logger().info(f"Turning on NIR probe LED {request.led}")

                # Turn on photodiode timer
                self.photodiode_light_blank.reset()
                self.stage = self.STAGE_LIGHT_BLANK

            case _:
                self.get_logger().error(f"Invalid LED request made: {request.led}")
                return response


        self.frame = jcan.Frame(self.NIR_PROBE_ID, [message]) # Save frame to turn on light at the end of light blanking

        response.success = True

        self.led = request.led

        self.get_logger().debug(f"Activating photodiode {request.led}")

        self.publish_msg(-1, -1, False, True) # -1 to not consider this value

        return response


    def photodiode_light_blank_callback(self):
        """
        One-shot callback that deactivates light blanking stage after period is over and activate calibration stage
        """
        try:
            self.get_logger().debug(f"Sending {self.frame}")
            self.bus.send(self.frame)

        except Exception as e:
            self.get_logger().error(f"Failed to send led command over CAN: {e}")


        self.photodiode_light_blank_timer.cancel()
        self.stage = self.STAGE_CALIBRATION

        self.average_data[0] = fmean(self.data[0])
        self.standard_deviation[0] = stdev(self.data[0])

        self.get_logger().debug(f"Entering calibration mode\nSending light blank average to gui: {self.average_data[0]}")

        self.publish_msg(self.average_data[0], self.standard_deviation[0], True, True)

        self.photodiode_calibration_timer.reset()


    def photodiode_calibration_callback(self):
        """
        One-shot callback that deactivates calibration stage after period is over and activates online stage
        """
        self.photodiode_calibration_timer.cancel()
        self.stage = self.STAGE_ONLINE
        
        self.average_data[1] = fmean(self.data[1])
        self.standard_deviation[1] = stdev(self.data[1])

        self.get_logger().debug(f"Entering online mode\nSending calibration mode average to gui: {self.average_data[1]}")

        self.publish_msg(self.average_data[1], self.standard_deviation[1], True, True)

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

        self.publish_msg(light_difference, self.standard_deviation[2], False, False)

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
