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
AUTHOR(S):   Brandon Chung, Felicity Matthews
CREATION:    14/01/2025
EDITED:      06/03/2025
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
import statistics
import rclpy, jcan
from rclpy.node import Node

# import custom messages
from nova_interfaces.msg import NIRProbeData
from nova_interfaces.srv import TakeNIRProbeReading

class NIRProbePublisher(Node):

    # Selecting CAN
    CAN_BUS = "can1"
    CAN_BUS_PARAM = "can_bus"

    # CARD IDs
    NIR_PROBE_ID = 0x0F0

    NIR_RECEIVE_ID = [
        PHOTODIODE1_ID := 0x4F1,
        PHOTODIODE2_ID := 0x4F0,
    ]

    # Commands
    LED_COMMANDS = [
        TURN_LED1_ON := 0x02,
        TURN_LED2_ON := 0x03,
    ]

    # All active photodiode states 
    PHTODIODE_STATES = [
        PHOTODIODE_OFF := (0).to_bytes(1, "big"), # Turns off both photodiodes
        PHOTODIODE1_ON := (1).to_bytes(1, "big"), # Exclusively turns on photodiode 1 and LED 1
        PHOTODIODE2_ON := (2).to_bytes(1, "big"), # Exclusively turns on photodiode 2 and LED 2
    ]

    # Timings
    SEND_INTERVAL = 0.1
    TIMEOUT_PERIOD = 15
    TIMEOUT_PARAM_NAME = "timeout"

    DEFAULT_COUNT = 30
    COUNT_PARAM_NAME = "count"

    def __init__(self):

        # Initialise publisher
        super().__init__('nir_probe_publisher')
        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("NIR Probe Publisher starting")
        self.publisher = self.create_publisher(NIRProbeData, '/science/nir_probe_data', 10)

        # Initialise parameters
        self.declare_parameter(self.TIMEOUT_PARAM_NAME, self.TIMEOUT_PERIOD)
        self.declare_parameter(self.COUNT_PARAM_NAME, self.DEFAULT_COUNT)

        # Initialise CAN bus
        self.declare_parameter(self.CAN_BUS_PARAM, self.CAN_BUS)
        self.bus = jcan.Bus()

        # Only accept inputs being delivered to receiving CAN IDs
        self.bus.set_id_filter(self.NIR_RECEIVE_ID)
        self.bus.add_callback(self.PHOTODIODE1_ID, self.read_data_callback)
        self.bus.add_callback(self.PHOTODIODE2_ID, self.read_data_callback)

        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)

        # Initialise service for taking commands
        self.command_service = self.create_service(TakeNIRProbeReading, '/science/take_nir_probe_reading', self.command_service_callback)

        # Initialise timers
        self.timeout_timer = self.create_timer(self.get_parameter(self.TIMEOUT_PARAM_NAME).value, self.update_last_readings, autostart=False)
        self.last_read_timer = self.create_timer(0.1, self.publish_msg)
        self.timer_jcan_spin = self.create_timer(self.SEND_INTERVAL, self.bus.spin)
        self.timer_poll_nir = self.create_timer(0.01, self.nir_poll_callback, autostart=False)

        # Internal State
        self.readings = []
        self.active_photodiode = self.PHOTODIODE_OFF
        self.last_led_on = self.PHOTODIODE_OFF
        self.last_average = 0
        self.last_stdv = 0
        self.last_count = 0

        self.get_logger().info(f"NIR Probe Publisher started on {self.get_parameter(self.CAN_BUS_PARAM).value}")

    """
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Support Functions
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """

    def publish_msg(self):
        """
        Publish data to GUI
        """
        msg = NIRProbeData()
        msg.data = self.last_average
        msg.stdv = self.last_stdv
        msg.count = self.last_count
        msg.led = self.last_led_on
        msg.status = self.active_photodiode
        self.get_logger().debug(f"Publishing {msg}")
        self.publisher.publish(msg)

    def request_reading(self, led):
        """
        Requests a reading from the NIR Probe
        """
        message = None
        if led == self.PHOTODIODE1_ON:
            message = self.TURN_LED1_ON
        elif led == self.PHOTODIODE2_ON:
            message = self.TURN_LED2_ON

        if message is None:
            self.get_logger().error(f"Invalid NIR probe request made: {led}")
            return False

        # Turn LED and photodiode on
        self.get_logger().info(f"Turning on NIR probe LED and photodiode {led}")

        frame = jcan.Frame(self.NIR_PROBE_ID, [message])
        self.active_photodiode = led

        try:
            self.get_logger().debug(f"Sending {frame}")
            self.bus.send(frame) # Turn lights on
            return True

        except Exception as e:
            self.get_logger().error(f"Failed to send NIR probe command over CAN: {e}")

        return False

    def update_last_readings(self):
        """
        Do calculations on data and update last variables
        """
        self.timeout_timer.cancel()

        # update last readings
        self.get_logger().info(f"Recorded readings: {self.readings}")
        self.last_count = len(self.readings)
        self.last_led_on = self.active_photodiode

        if len(self.readings) > 1:
            self.last_average = statistics.mean(self.readings)
            self.last_stdv = statistics.stdev(self.readings)
        elif len(self.readings) == 1:
            self.last_average = self.readings[0]
            self.last_stdv = 0
        else:
            self.last_average = 0
            self.last_stdv = 0

        # reset variables
        self.readings = []
        self.active_photodiode = self.PHOTODIODE_OFF

        self.publish_msg()


    """
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ROS2 Functions
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """

    def read_data_callback(self, frame: jcan.Frame):
        """
        Read data received from the photodiodes
        """
        # Receive data if corresponding LED is on
        if frame.id not in self.NIR_RECEIVE_ID: 
            self.get_logger().warn(f"Received unknown frame {frame}")
            return

        if self.active_photodiode == self.PHOTODIODE_OFF:
            self.get_logger().warn(f"Received frame when active_photodiode is off {frame}")
            return

        if frame.id == self.PHOTODIODE1_ID and self.active_photodiode != self.PHOTODIODE1_ON:
            self.get_logger().warn(f"Received frame from wrong photodiode {frame}")
            self.timer_poll_nir.reset()
            return
        if frame.id == self.PHOTODIODE2_ID and self.active_photodiode != self.PHOTODIODE2_ON:
            self.get_logger().warn(f"Received frame from wrong photodiode {frame}")
            self.timer_poll_nir.reset()
            return

        self.get_logger().debug(f"Received {hex(frame.id)} {frame.data}")

        # add reading to list
        self.readings.append(int.from_bytes(frame.data, "big"))

        # update last readings if reached end
        if len(self.readings) >= self.get_parameter(self.COUNT_PARAM_NAME).value:
            self.update_last_readings()
            return

        self.timer_poll_nir.reset()

    def command_service_callback(self, request, response):
        """
        Run commands from GUI on NIR Probe
        """
        response.success = self.request_reading(request.led)
        self.timeout_timer.reset()
        return response

    def nir_poll_callback(self):
        self.request_reading(self.active_photodiode)
        self.timer_poll_nir.cancel()    

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
