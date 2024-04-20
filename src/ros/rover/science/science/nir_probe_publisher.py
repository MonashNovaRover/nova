#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 receiver code for the
nir probe publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: nir_probe_publisher
TOPICS:
  - /science/nir_probe_data [NIRProbeData]
    [Published]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     science
AUTHOR(S):   Bailey Chessum
CREATION:    4/02/2024
EDITED:      15/02/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import logging
import rclpy, jcan
from rclpy.node import Node

# import custom messages
from nova_interfaces.msg import NIRProbeData
from nova_interfaces.srv import SetNIRProbeLED

class NIRProbePublisher(Node):
    CAN_BUS = "can1"
    # Card IDs
    NIR_PROBE_ID = 0x0F0
    CARD_ID_RECEIVE = 0x4F1
    # Command data
    NIR_PROBE_LED_ON = 0x01
    NIR_PROBE_LED_OFF = 0x02
    NIR_PROBE_READ = 0x03

    CAN_BUS_PARAM = "can_bus"

    LED_BYTES_OFF = (0).to_bytes(1, "big")
    LED_BYTES_ON = (1).to_bytes(1, "big")

    def __init__(self):
        super().__init__('nir_probe_publisher')

        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("NIR Probe Publisher starting")

        self.declare_parameter(self.CAN_BUS_PARAM, self.CAN_BUS)

        self.led = self.LED_BYTES_OFF
        self.value = 0

        self.publisher = self.create_publisher(NIRProbeData, '/science/nir_probe_data', 10)

        self.led_service = self.create_service(SetNIRProbeLED, '/science/set_nir_probe_led', self.led_service_callback)

        self.bus = jcan.Bus()
        self.bus.set_id_filter_mask(self.CARD_ID_RECEIVE, 0xFFF)
        self.bus.add_callback(self.CARD_ID_RECEIVE, self.read_data_callback)

        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)

        self.timer = self.create_timer(0.1, self.send_read_command_callback)
        self.timer_jcan_spin = self.create_timer(0.01, self.bus.spin)

        self.get_logger().info(f"NIR Probe Publisher started on {self.get_parameter(self.CAN_BUS_PARAM).value}")

    def send_read_command_callback(self):
        """
        Sends the read command to the NIR probe
        """
        frame = jcan.Frame(self.NIR_PROBE_ID, [self.NIR_PROBE_READ])
        
        try:
            self.bus.send(frame)
        except Exception as e:
            print(e)
            self.get_logger().error("Failed to send read command over CAN")

    def read_data_callback(self, frame: jcan.Frame):
        """
        Callback for when data is received from the NIR probe
        """
        self.get_logger().debug(f"Received {hex(frame.id)} {frame.data}")

        if frame.id != self.CARD_ID_RECEIVE:
            self.get_logger().warn(f"Received unknown frame {frame}")
            return

        self.value = int.from_bytes(frame.data, "big")

        msg = NIRProbeData()
        msg.data = self.value
        msg.led = self.led
        self.get_logger().debug(f"Publishing {msg}")
        self.publisher.publish(msg)

    def led_service_callback(self, request, response):
        """
        Callback to turn the NIR probe LED on or off
        """
        frame = None
        if request.led == self.LED_BYTES_OFF:
            self.get_logger().info("Turning NIR probe LED OFF")
            self.led = self.LED_BYTES_OFF
            frame = jcan.Frame(self.NIR_PROBE_ID, [self.NIR_PROBE_LED_OFF])
        else:
            self.get_logger().info("Turning NIR probe LED ON")
            self.led = self.LED_BYTES_ON
            frame = jcan.Frame(self.NIR_PROBE_ID, [self.NIR_PROBE_LED_ON])

        try:
            self.get_logger().debug(f"Sending {frame}")
            self.bus.send(frame)
            response.success = True
        except Exception as e:
            self.get_logger().error(f"Failed to send led command over CAN: {e}")
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
