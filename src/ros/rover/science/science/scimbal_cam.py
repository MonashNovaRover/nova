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

import rclpy
from enum import Enum

from nova_interfaces.srv import MoveScimbalCam
from python_control.ControllerNode import ControllerNode
from python_control.controls.ContinuousOneAxisPositionControl import ContinuousOneAxisPositionControl
from python_control.controllers.JonoPositionController import JonoPositionController

class ScimbalCamServos(Enum):
    TILT = 0
    PAN = 1

class ScimbalCamNode(ControllerNode):
    SERVICE_TYPE = MoveScimbalCam
    SERVICE_NAME = '/science/scimbal_cam_service'

    # can bus
    CAN_BUS = "can1"

    # card IDs
    SERVO_IDS = [0x0A0, 0x0A0]
    SERVO_CONTROL_NAMES = ["TILT", "PAN"]

    # command data
    MOVE_SERVO_COMMANDS = [0x05, 0x06]

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

        self.scimbal_cam: list[ContinuousOneAxisPositionControl] = [None] * len(self.SERVO_IDS)
        self.scimbal_cam_controllers: list[JonoPositionController] = [None] * len(self.SERVO_IDS)

        for i in range(len(self.SERVO_IDS)):
            ## Create CONTROLS
            scimbal_cam_control = ContinuousOneAxisPositionControl(
                logger=logger,
                max_angle=self.MAX_ANGLES[i],
            )
            scimbal_cam_control.set_position(self.START_ANGLES[i])
            self.scimbal_cam[i] = scimbal_cam_control

            ## Create CONTROLLERS
            self.scimbal_cam_controllers[i] = JonoPositionController(
                logger=logger,
                bus=self.bus,
                pos_command=self.MOVE_SERVO_COMMANDS[i],
                frame_id=self.SERVO_IDS[i],
                control=scimbal_cam_control,
                max_value=self.MAX_VALUE,
            )

            ## Add the CONTROLLERS to the node's controllers
            self.add_controller(self.SERVO_CONTROL_NAMES[i], self.scimbal_cam_controllers[i])

        ## Start the CAN bus
        self.start_can()

    def request_servo(self, request, response):
        try:
            for i in range(len(request.angles)):
                angle = request.angles[i]
                self.scimbal_cam[i].displace(angle)
            self.get_logger().info(f"Scimbal Cam angles updated: TILT: {self.scimbal_cam[0].get_goal_position()}, PAN: {self.scimbal_cam[1].get_goal_position()}, request: {request.angles}")
            response.success = True
        except Exception as e:
            self.get_logger().error(f"Scimbal Cam angle update request {request.angles} interrupted by error: {str(e)}")
            response.success = False
        return response


def main():
    rclpy.init()
    scimbal_cam = ScimbalCamNode()
    rclpy.spin(scimbal_cam)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
