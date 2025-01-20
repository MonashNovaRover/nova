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
    ]

    # All active photodiode states 
    PHTODIODE_STATES = [
        PHOTODIODE_OFF := (0).to_bytes(1, "big"), # Turns off both photodiodes
        PHOTODIODE1_ON := (1).to_bytes(1, "big"), # Exclusively turns on photodiode 1 and LED 1
        PHOTODIODE2_ON := (2).to_bytes(1, "big"), # Exclusively turns on photodiode 2 and LED 2
    ]

    # Timings
    SEND_INTERVAL         = 0.01
    READ_INTERVAL         = SEND_INTERVAL * 10

    # Number of seconds between each "last reading"
    last_reading_interval = SEND_INTERVAL * 300 # 3 seconds

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
        self.last_read_timer = self.create_timer(0.1, self.last_read_callback)
        self.timer_jcan_spin = self.create_timer(self.SEND_INTERVAL, self.bus.spin)

        self.active_photodiode = self.PHOTODIODE_OFF
        self.last_reading = 0;

        self.get_logger().info(f"NIR Probe Publisher started on {self.get_parameter(self.CAN_BUS_PARAM).value}")

    """
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Support Functions
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """

    def publish_msg(self, data: float):
        """
        Publish data to GUI
        """
        msg = NIRProbeData()
        msg.active_photodiode = self.active_photodiode
        msg.data = data
        self.get_logger().debug(f"Publishing {msg}")
        self.publisher.publish(msg)

    """
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ROS2 Functions
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """

    def last_read_callback(self):
        """
        Send last reading at a given interval
        """
        self.publisher.debug("Publishing last reading")
        self.publish_msg(self.last_reading)


    def read_data_callback(self, frame: jcan.Frame):
        """
        Read data received from the photodiodes
        """
        # Receive data if corresponding LED is on
        if frame.id not in self.NIR_RECEIVE_ID: 
            self.get_logger().warn(f"Received unknown frame {frame}")
            return

        self.get_logger().debug(f"Received {hex(frame.id)} {frame.data}")

        value = self.last_reading = int.from_bytes(frame.data, "big")

        self.publish_msg(value)


    def command_service_callback(self, request, response):
        """
        Run commands from GUI on NIR Probe
        """
        message = int.from_bytes(request.command, "big")

        # Check which led state NIR Probe publisher is in
        if request.command not in self.PHOTODIODE_STATES:
            self.get_logger().error(f"Invalid NIR probe request made: {request.command}")
            response.success = False
            return response

        # Turn LED and photodiode on
        self.get_logger().info(f"Turning on NIR probe LED and photodiode {message}")

        frame = jcan.Frame(self.NIR_PROBE_ID, [message])
        self.command = request.command

        try:
            self.get_logger().debug(f"Sending {frame}")
            self.bus.send(frame) # Turn lights on
            response.success = True

        except Exception as e:
            self.get_logger().error(f"Failed to send NIR probe command over CAN: {e}")
            response.success = False

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
