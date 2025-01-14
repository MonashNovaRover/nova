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
EDITED:      14/01/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This package bridges turning on LEDs with
collecting NIR photodiode data.

The only commands to send this publisher are:
    1. Turn on LED 1
    2. Turn on LED 2
    3. Turn of LEDs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import logging
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
        ID_PHOTODIODE1 := 0x4F0,
        ID_PHOTODIODE2 := 0x4F1,
    ]

    # Commands
    LED_COMMANDS = [
        TURN_LED1_ON := 0x01,
        TURN_LED2_ON := 0x02,
        TURN_LED_OFF := 0x03,
    ]

    PHOTODIODE_COMMANDS = [
        READ_PHOTODIODE1 := 0x04,
        READ_PHOTODIODE2 := 0x05
    ]

    # All states NIR Probe may be in
    LEDS = [
        LED1_ON, # Turns on LED 1, Turns off LED 2
        LED2_ON, # Turns on LED 2, Turns off LED 1
        LED_OFF, # Turns off both LEDs
    ] = list(map(lambda i: (i).to_bytes(1, "big"), LED_COMMANDS))

    # Timings
    SEND_INTERVAL                 = 0.01
    READ_INTERVAL                 = 0.1 # Keep this as a mutliple of send interval
    PHOTODIODE_CALIBRATION_PERIOD = 1

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
        for can_id in self.NIR_RECEIVE_ID:
            self.bus.add_callback(can_id, self.read_data_callback)

        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)

        # Initialise service for taking commands
        self.led = self.LED_OFF
        self.led_service = self.create_service(SetNIRProbeLED, '/science/set_nir_probe_led', self.led_service_callback)

        # Initialise timers
        self.photodiode_timers = [
            self.photodiode1_timer,
            self.photodiode2_timer,
        ] = [
            self.create_timer(self.READ_INTERVAL, self.send_read_command_callback),
            self.create_timer(self.READ_INTERVAL, self.send_read_command_callback),
        ]

        for timer in self.photodiode_timers:
            timer.cancel()
        
        self.timer_jcan_spin = self.create_timer(self.SEND_INTERVAL, self.bus.spin)

        self.calibration_timer = self.create_timer(self.PHOTODIODE_CALIBRATION_PERIOD, self.calibration_callback)
        self.calibration_timer.cancel()
        self.calibrating = True

        self.get_logger().info(f"NIR Probe Publisher started on {self.get_parameter(self.CAN_BUS_PARAM).value}")

    def send_read_command_callback(self):
        """
        Sends the read command to the NIR probe
        """
        try:
            self.bus.send(jcan.Frame(self.NIR_PROBE_ID, [int.from_bytes(self.led, "big")]))

        except Exception as e:
            print(e)
            self.get_logger().error(f"Failed to send read command over CAN for photodiode {self.led}")

    def read_data_callback(self, frame: jcan.Frame):
        """
        Callback for when data is received from the NIR probe
        """
        self.get_logger().debug(f"Received {hex(frame.id)} {frame.data}")

        # Receive data if corresponding LED is on
        match frame.id:

            case self.ID_PHOTODIODE1 | self.ID_PHOTOIODE2:
                msg = NIRProbeData()
                msg.data = int.from_bytes(frame.data, "big")
                msg.led = self.led
                msg.calibrating = self.calibrating

                self.get_logger().debug(f"Publishing {msg}")
                self.publisher.publish(msg)

            case _:
                self.get_logger().warn(f"Received unknown frame {frame}")

    def calibration_callback(self):
        """
        One-shot callback that deactivates calibration after period is over
        """
        self.calibrating = False
        self.calibration_timer.cancel()


    def led_service_callback(self, request, response):
        """
        Callback to turn the NIR probe LEDs on or off
        """
        response.success = False
        self.calibrating = True
        message = int.from_bytes(request.led, "big")

        # Check which led state NIR Probe publisher is in
        match request.led:

            case self.LED_OFF:
                self.get_logger().debug("Turning off LEDs and photodiodes.")

            case self.LED_1_ON | self.LED2_ON:
                # Turn LED on
                self.calibration_timer.reset()
                self.get_logger().info(f"Turning on NIR probe LED {request.led}")

                # Turn on photodiode for led
                self.photodiode_timers[request.led-1].reset()

                # Turn off other photodiode
                self.photodiode_timers[request.led%2].cancel()

            case _:
                self.get_logger().error(f"Invalid LED request made: {request.led}")
                response.success = False
                return response

        try:
            led_frame = jcan.Frame(self.NIR_PROBE_ID, [message])
            self.get_logger().debug(f"Sending {led_frame}")
            self.bus.send(led_frame)
            response.success = True

        except Exception as e:
            self.get_logger().error(f"Failed to send led command over CAN: {e}\nTurning NIR probe LEDs OFF as fail safe")
            self.bus.send(jcan.Frame(self.NIR_PROBE_ID, [self.TURN_LED_OFF]))
            response.success = False

        self.led = request.led
        return response


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
