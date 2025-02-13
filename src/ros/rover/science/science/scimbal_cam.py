#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Scimbal Cam zoom with respective servos
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ScimbalCamNode
TOPICS: None
SERVICES: 
    - server: /science/microscope_servo_service [MoveMicroscopeServo]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Felicity Matthews
CREATION:	12/02/2025
EDITED:		12/02/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy, jcan, logging
from rclpy.node import Node
from enum import Enum

from nova_interfaces.srv import MoveScimbalCam

class ScimbalCamServos(Enum):
    TILT = 0
    PAN = 1

class ScimbalCamNode(Node):
    SERVICE_TYPE = MoveScimbalCam
    SERVICE_NAME = '/science/scimbal_cam_service'

    # can bus
    CAN_BUS = "can1"

    # card IDs
    SERVO_IDS = [0x011, 0x022]

    # command data
    MOVE_SERVO_COMMAND = 0x0D

    # angle
    MIN_ANGLES = [0, 0]
    INITIAL_ANGLES = [0, 0]
    MAX_ANGLES = [180, 360]

    MAX_VALUE = 0x9B
    MIN_VALUE = 0x00

    CAN_BUS_PARAM = "can_bus"
    SERVO_IDS_PARAM = "servo_ids"

    def __init__(self):
        super().__init__("scimbal_cam")

        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Scimbal Cam starting")

        self.declare_parameter(self.CAN_BUS_PARAM, self.CAN_BUS)
        self.declare_parameter(self.SERVO_IDS_PARAM, self.SERVO_IDS)

        self.service = self.create_service(ScimbalCamNode.SERVICE_TYPE, ScimbalCamNode.SERVICE_NAME, self.request_servo)

        self.current_angles = ScimbalCamNode.INITIAL_ANGLES.copy()

        self.bus = jcan.Bus()
        
        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)
        self.timer_can_commands = self.create_timer(0.2, self.send_can_commands)

        self.get_logger().info(f"Scimbal Cam started on {self.get_parameter(self.CAN_BUS_PARAM).value}")


    def move_servo(self, target_angles):
        for i in range(len(target_angles)):
            try:
                servo_frame = jcan.Frame(self.get_parameter(self.SERVO_IDS_PARAM).value[i], [ScimbalCamNode.MOVE_SERVO_COMMAND, int(target_angles[i] / self.MAX_ANGLES[i] * self.MAX_VALUE)])
                self.bus.send(servo_frame)
            except Exception as e:
                self.get_logger().error(f"Failed to send scimbal cam servo ({ScimbalCamServos(i).name} angle: {target_angles[i]}) CAN command: {str(e)}")
                        
    def request_servo(self, request, response):
        for i in range(len(request.angles)):
            angle = request.angles[i]
            new_angle = self.current_angles[i] + angle
            new_angle = min(new_angle, ScimbalCamNode.MAX_ANGLES[i])
            new_angle = max(new_angle, ScimbalCamNode.MIN_ANGLES[i])
            self.current_angles[i] = new_angle

        try:
            self.move_servo(self.current_angles)
            self.get_logger().info(f"Scimbal Cam angles updated to {self.current_angles}, request: {request.angles}")
            response.success = True
        except Exception as e:
            self.get_logger().error(f"Scimbal Cam angle update request {request.angle} interrupted by error: {str(e)}")
            response.success = False
        return response

    def send_can_commands(self):
        self.move_servo(self.current_angles)

def main():
    rclpy.init()
    microscopeServo = ScimbalCamNode()
    rclpy.spin(microscopeServo)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
