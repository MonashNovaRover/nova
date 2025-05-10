#!/usr/bin/env python3


import logging
import rclpy
from rclpy.node import Node
import jcan

from nova_interfaces.srv import KilnCommand


class KilnServer(Node):
    # Jono Card IDs
    KILN_CARD_SEND_IDS = 0x0A0
    KILN_TEMP_FEEDBACK_ID = 0x4E0
    # Kiln Command
    KILN_POWER_COMMAND = 0x07
    # Kiln Power States
    KILN_OFF = 0x02
    KILN_ON = 0x01
    # Kiln Sensor IDs
    KILN_SENSOR_IDS = [0x03]
    # ROS Params
    CAN_BUS_PARAM = "can_bus"
    KILN_TEMP_CONVERSION_PARAM = "science_temp_conversion"
    # ROS Topics
    KILN_DATA_TOPIC = "/science/kiln_data"
    # ROS Services
    KILN_COMMAND_SERVICE = "/science/kiln_command"
    # Default target
    DEFAULT_TARGET_TEMP = 25

    def __init__(self):
        super().__init__('kiln_server')

        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Kiln Server starting")

        # The calculations are currently performed on the arduino side to this is set to false
        # If the calculations are to be performed on the ROS side, this should be set to true
        self.declare_parameter(KilnServer.CAN_BUS_PARAM, "can1")

        # subscriber to polling status
        self.service = self.create_service(KilnCommand, KilnServer.KILN_COMMAND_SERVICE, self.command_callback)

        # initialise the can bus
        self.bus = jcan.Bus()

        # create timers
        self.can_spin_timer = self.create_timer(0.05, self.bus.spin)

        self.is_active = False
        self.is_clockwise = False

        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)

        self.get_logger().info(f"Kiln Server started on {self.get_parameter(self.CAN_BUS_PARAM).value}")

    def command_callback(self, request, response):
        """
        Callback for the kiln command service
        """
        try:
            self.get_logger().info(f"Carousel service request received: {request}")
            if request.state and not self.is_active:
                frame = jcan.Frame(id=self.KILN_CARD_SEND_IDS, data=[0x01, 0])
                self.is_active = True
                self.get_logger().info(f"Carousel turned on")
            elif not request.state and self.is_active:
                frame = jcan.Frame(id=self.KILN_CARD_SEND_IDS, data=[0x02, 0])
                self.is_active = False
                self.get_logger().info(f"Carousel turned off")
            elif request.target < 0:
                self.is_clockwise = not self.is_clockwise
                if self.is_clockwise:
                    frame = jcan.Frame(id=self.KILN_CARD_SEND_IDS, data=[0x00, 0x01])
                else:
                    frame = jcan.Frame(id=self.KILN_CARD_SEND_IDS, data=[0x00, 0x00])
                self.get_logger().info(f"Carousel changing directions, now: {"CLOCKWISE" if self.is_clockwise else "ANTICLOCKWISE}"}")
            else:
                frame = jcan.Frame(id=self.KILN_CARD_SEND_IDS, data=[0x03, request.target])
                self.get_logger().info(f"Carousel moving {request.target} steps")

            self.bus.send(frame)

            self.get_logger().info(f"Stepper Status = {self.is_active}")
            response.success = True
        except Exception as e:
            self.get_logger().error(f"Failed to process carousel service request: {str(e)}")
            response.success = False

        return response




def main():
    rclpy.init()
    server_node = KilnServer()
    rclpy.spin(server_node)
    server_node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
