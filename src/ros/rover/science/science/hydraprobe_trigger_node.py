#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 trigger node for the
Hydraprobe moisture sensor.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: hydraprobe_trigger_node
TOPICS:
  - /science/hydraprobe_data [HydraprobeData]
    [Published]
SERVICES:
  - /science/request_hydraprobe_reading [Trigger]
    [Called to take a reading]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     science
AUTHOR(S):   Kuhu Tosniwal
CREATION:    08/05/2025
EDITED:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This node exposes a Trigger service that performs
a one-time moisture reading and publishes it.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import time
import logging
import rclpy
from rclpy.node import Node

# ROS2 service/message imports
from std_srvs.srv import Trigger
from nova_interfaces.msg import HydraprobeData

# Hardware abstraction
from urc_hydraprobe import NewHydraprobeTransceiver

class HydraprobeTriggerNode(Node):

    # Serial port and soil type parameters
    # Should be the orin port
    DEFAULT_PORT = "/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_AU0K3IZ3-if00-port0"
    # MY PORT
    # DEFAULT_PORT = "/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_AQ018NTV-if00-port0"
    DEFAULT_SOIL = "sand"

    def __init__(self):
        super().__init__('hydraprobe_trigger_node')
        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Hydraprobe Trigger Node starting")

        # Declare parameters
        self.declare_parameter("port", self.DEFAULT_PORT)
        self.declare_parameter("soil", self.DEFAULT_SOIL)

        # Create publisher
        self.publisher = self.create_publisher(HydraprobeData, "/science/hydraprobe_data", 10)

        self.last_reading = None  # to store the last HydraprobeData message

        # Timer to keep publishing last reading (every 0.5s)
        self.republish_timer = self.create_timer(0.5, self.publish_last_reading)

        # Create service
        self.trigger_service = self.create_service(
            Trigger,
            "/science/request_hydraprobe_reading",
            self.handle_trigger_request
        )

        # Initialise transceiver
        try:
            self.transceiver = NewHydraprobeTransceiver(
                port=self.get_parameter("port").value,
                logger=self.get_logger()
            )
            self.transceiver.set_soil_type(self.get_parameter("soil").value)
            self.get_logger().info("Hydraprobe Trigger Node ready")
        except Exception as e:
            self.get_logger().error(f"Failed to initialise hydraprobe: {e}")
            raise e

    """
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ROS2 Functions
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """

    def handle_trigger_request(self, request, response):
        """
        Called when GUI requests a new reading.
        """
        self.get_logger().info(" Trigger received: Taking hydraprobe reading...")

        try:
            values = self.transceiver.read_all()

            if values is None:
                raise RuntimeError("Sensor returned no data")

            self.get_logger().info("Received service call — Simulated reading.")

            msg = HydraprobeData()
            msg.temperature = float(values[0])
            msg.moisture = float(values[1])
            msg.conductivity = float(values[2])
            msg.dielectric = float(values[3])

#             msg.temperature = 25.0
#             msg.moisture = 25.0
#             msg.conductivity = 25.0
#             msg.dielectric = 25.0

            self.last_reading = msg  # save for repeated publishing
            self.publisher.publish(msg)

            self.get_logger().info(f" Published: T={msg.temperature}°C, M={msg.moisture}%, C={msg.conductivity}, D={msg.dielectric}")

            response.success = True
            response.message = "Hydraprobe reading taken and published."

        except Exception as e:
            self.get_logger().error(f" Reading failed: {e}")
            response.success = False
            response.message = f"Error: {str(e)}"

        return response

    def publish_last_reading(self):
        """
        Continuously re-publish the last reading so new subscribers always get it.
        """
        if self.last_reading:
            self.publisher.publish(self.last_reading)
        else:
            self.get_logger().debug("No hydraprobe reading yet to publish.")


"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Main
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

def main(args=None):
    rclpy.init(args=args)
    node = HydraprobeTriggerNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
