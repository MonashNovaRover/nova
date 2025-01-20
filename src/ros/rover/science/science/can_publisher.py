#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 receiver code for the
can message publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: nir_probe_publisher
TOPICS:
  - /science/can [CANMessage]
    [Published]
SERVICES
  - /science/can_send [SendCANMessage]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     science
AUTHOR(S):   Bailey Chessum
CREATION:    20/01/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import logging
import rclpy, jcan
from rclpy.node import Node

# import custom messages
from nova_interfaces.msg import CANMessage
from nova_interfaces.srv import SendCANMessage


class CANPublisher(Node):
    CAN_BUS = "can0"

    CAN_BUS_PARAM = "can_bus"

    def __init__(self):
        super().__init__('can_publisher')

        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("CAN Publisher starting")

        self.declare_parameter(self.CAN_BUS_PARAM, self.CAN_BUS)

        self.publisher = self.create_publisher(CANMessage, '/science/can', 10)

        self.send_service = self.create_service(SendCANMessage, '/science/can_send', self.send_service_callback)

        self.bus = jcan.Bus()
        all_ids = [x for x in range(2**2)]

        for cid in all_ids:
            self.get_logger().info(f"CAN Subscriber to 0")
            self.bus.set_id_filter_mask(cid, 0xFFF)
            self.bus.add_callback(cid, self.read_data_callback)

        # The number suffix for the can bus (can0 -> 0, vcan0 -> 0, can1 -> 1, etc).
        self.can_bus_number = int(self.get_parameter(self.CAN_BUS_PARAM).value[-1])

        self.get_logger().info(f"CAN Publisher on {self.get_parameter(self.CAN_BUS_PARAM).value} ({self.can_bus_number})")
        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)

        self.timer_jcan_spin = self.create_timer(0.01, self.bus.spin)

        self.get_logger().info(f"CAN Publisher started on {self.get_parameter(self.CAN_BUS_PARAM).value}")

    def read_data_callback(self, frame: jcan.Frame):
        """
        Callback for when data is received from the CAN bus
        """
        self.get_logger().debug(f"Received {hex(frame.id)}#{frame.data}")

        msg = CANMessage()
        msg.bus = self.can_bus_number
        msg.id = frame.id
        msg.data = frame.data
        self.publisher.publish(msg)

    def send_service_callback(self, request, response):
        """
        Callback to turn the NIR probe LED on or off
        """
        if request.bus != self.can_bus_number:
            self.get_logger().debug(f"{request.bus} != {self.can_bus_number}")
            return

        frame = jcan.Frame(request.id, request.data)

        try:
            self.get_logger().debug(f"Sending {hex(request.id)}#{''.join([hex(x) for x in frame.data])}")
            self.bus.send(frame)
            response.success = True
        except Exception as e:
            self.get_logger().error(f"Failed to send led command over CAN: {e}")
            response.success = False

        return response


# The main code that executes when starting
def main(args=None):
    # Create the publisher
    rclpy.init(args=args)
    publisher = CANPublisher()
    rclpy.spin(publisher)

    #  Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__ == "__main__":
    main()
