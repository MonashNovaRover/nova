#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Scimbal Cam movement with respective servos
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
EDITED:		09/03/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy, jcan, logging
from enum import Enum

from nova_interfaces.srv import MoveScimbalCam
from python_control.ControllerNode import ControllerNode

class ScimbalCamServos(Enum):
    TILT = 0
    PAN = 1

class ScimbalCamNode(ControllerNode):
    SERVICE_TYPE = MoveScimbalCam
    SERVICE_NAME = '/science/scimbal_cam_service'

    # can bus
    CAN_BUS = "can1"

    # card IDs
    SERVO_IDS = [0x011, 0x022]

    # command data
    MOVE_SERVO_COMMAND = 0x10

    # angle
    # position 0 = TILT, 1 = PAN
    START_ANGLES = [90, 180] # in the middle
    MAX_ANGLES = [180, 360]

    MAX_VALUE = 0xFF

    SERVO_IDS_PARAM = "servo_ids"

    def __init__(self):
        super(ScimbalCamNode, self).__init__(name="scimbal_cam", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.declare_parameter(self.SERVO_IDS_PARAM, self.SERVO_IDS)

        ## Add Service
        self.service = self.create_service(ScimbalCamNode.SERVICE_TYPE, ScimbalCamNode.SERVICE_NAME, self.request_servo)

        ## Create CONTROLS
        self.scimbal_cam = [
            ContinuousOneAxisPositionControl(
                logger=logger,
                max_angle=self.MAX_ANGLES[0],
            ),
            ContinuousOneAxisPositionControl(
                logger=logger,
                max_angle=self.MAX_ANGLES[1],
            ),
        ]
        self.scimbal_cam[0].set_position(self.START_ANGLES[0])
        self.scimbal_cam[1].set_position(self.START_ANGLES[1])

        ## Create CONTROLLERS
        self.scimbal_cam_controllers = [
            JonoPositionController(
                logger=logger,
                bus=self.bus,
                pos_command=self.MOVE_SERVO_COMMAND,
                frame_id=self.SERVO_ID[0],
                control=self.scimbal_cam[0],
                max_value=self.MAX_VALUE,
            ),
            JonoPositionController(
                logger=logger,
                bus=self.bus,
                pos_command=self.MOVE_SERVO_COMMAND,
                frame_id=self.SERVO_ID[1],
                control=self.scimbal_cam[1],
                max_value=self.MAX_VALUE,
            )
        ]

        ## Start the CAN bus
        self.start_can()

    def request_servo(self, request, response):
        try:
            for i in range(len(request.angles)):
                angle = request.angles[i]
                self.scimbal_cam[i].displace(angle)
                self.get_logger().info(
                    f"Scimbal Cam angles updated to {self.current_angles}, request: {request.angles}")
            self.get_logger().info(f"Scimbal Cam angles updated to {self.current_angles}, request: {request.angles}")
            response.success = True
        except Exception as e:
            self.get_logger().error(f"Scimbal Cam angle update request {request.angle} interrupted by error: {str(e)}")
            response.success = False
        return response


def main():
    rclpy.init()
    microscopeServo = ScimbalCamNode()
    rclpy.spin(microscopeServo)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
