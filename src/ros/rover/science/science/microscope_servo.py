#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control microscope zoom with respective servo
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: MicroscopeNode
TOPICS:
    - publisher: /science/microscope_servo_info [MicroscopeServoInfo]
SERVICES: 
    - server: /science/microscope_servo_service [MoveMicroscopeServo]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Connor Macdougall
CREATION:	15/03/2024
EDITED:		15/03/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy, jcan, logging
from rclpy.node import Node

from nova_interfaces.srv import MoveMicroscopeServo
from nova_interfaces.msg import MicroscopeServoInfo


class MicroscopeNode(Node):

    SERVICE_TYPE = MoveMicroscopeServo
    SERVICE_NAME = '/science/microscope_servo_service'
    TOPIC_TYPE = MicroscopeServoInfo
    TOPIC_NAME = '/science/microscope_servo_info'

    # can bus
    CAN_BUS = "can1"
    # card IDs
    MICROSCOPE_SERVO_ID = 0x0B0
    # command data
    MOVE_SERVO_COMMAND = 0x0D
    # angle
    MIN_ANGLE = 0
    INITIAL_ANGLE = 0
    MAX_ANGLE = 90

    MAX_VALUE = 0x9B
    MIN_VALUE = 0x00

    CAN_BUS_PARAM = "can_bus"

    def __init__(self):
        super().__init__("microscope_servo")

        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Microscope Servo starting")

        self.declare_parameter(self.CAN_BUS_PARAM, self.CAN_BUS)

        self.service = self.create_service(MicroscopeNode.SERVICE_TYPE, MicroscopeNode.SERVICE_NAME, self.request_servo)
        self.publisher = self.create_publisher(MicroscopeNode.TOPIC_TYPE, MicroscopeNode.TOPIC_NAME, 10)

        self.current_angle = MicroscopeNode.INITIAL_ANGLE

        self.bus = jcan.Bus()
        
        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)
        self.timer_can_commands = self.create_timer(0.2, self.send_can_commands)
        self.timer_publish_info = self.create_timer(0.1, self.publish_info)

        self.get_logger().info(f"Microscope Servo started on {self.get_parameter(self.CAN_BUS_PARAM).value}")


    def move_servo(self, target_angle):
        try:
            servo_frame = jcan.Frame(MicroscopeNode.MICROSCOPE_SERVO_ID , [MicroscopeNode.MOVE_SERVO_COMMAND, int(target_angle / self.MAX_ANGLE * self.MAX_VALUE)])
            self.bus.send(servo_frame)
        except Exception as e:
            self.get_logger().error(f"Failed to send microscope servo (angle: {target_angle}) CAN command: {str(e)}")
                        
    def request_servo(self, request, response):
        try:
            if MicroscopeNode.MIN_ANGLE <= request.angle <= MicroscopeNode.MAX_ANGLE:
                self.current_angle = request.angle
                self.move_servo(self.current_angle)
                self.get_logger().info(f"Microscope angle updated to {request.angle}")
                response.success = True
            else:
                self.get_logger().error(f"Invalid angle of {request.angle} sent. Angle must be between {MicroscopeNode.MIN_ANGLE} and {MicroscopeNode.MAX_ANGLE} (inclusive).")
                response.success = False
        except Exception as e:
            self.get_logger().error(f"Microscope angle update request {request.angle} interrupted by error: {str(e)}")
            response.success = False
        return response

    def send_can_commands(self):
        self.move_servo(self.current_angle)

    def publish_info(self):
        """
        Publishes the current angle of the microscope servo
        """
        try: 
            msg = MicroscopeServoInfo()
            msg.angle = self.current_angle
            self.publisher.publish(msg)
            self.get_logger().debug(f"Microscope servo angle {str(self.current_angle)} published")
        except Exception as e:
            self.get_logger().error(f"Failed to publish microscope servo data: {str(e)}")


def main():
    rclpy.init()
    microscopeServo = MicroscopeNode()
    rclpy.spin(microscopeServo)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
