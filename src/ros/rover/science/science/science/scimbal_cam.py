#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Scimbal Cam movement with respective servos
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ScimbalCamController
TOPICS: None
SERVICES:
    - server: /science/scimbal_cam_service [MoveScimbalCam]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 
AUTHOR(S):      Ivan Li
CREATION:       15/01/2026
EDITED:         15/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
import jcan

from science_interfaces.srv import MoveScimbalCam
from enum import Enum
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
from python_control2.hardware_interfaces import PositionalServoHardware


class ScimbalCamController(Controller):
    # Service type
    SERVICE_TYPE = MoveScimbalCam

    # Command interfaces
    tilt_cmd: Interface
    pan_cmd: Interface

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"ScimbalCamNode -- I have been __init__ialized")

        # Defining service name
        self.service_name = self.declare_parameter("service_name", "/science/scimbal_cam_service").value

        # Defining start, min, and max angles
        self.start_tilt_angle = self.declare_parameter("start_tilt_angle", 90).value
        self.start_pan_angle = self.declare_parameter("start_pan_angle", 180).value

        self.min_tilt_angle = self.declare_parameter("min_tilt_angle", 0).value
        self.min_pan_angle = self.declare_parameter("min_pan_angle", 0).value

        self.max_tilt_angle = self.declare_parameter("max_tilt_angle", 180).value
        self.max_pan_angle = self.declare_parameter("max_pan_angle", 360).value        

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Save references to interfaces
        self.logger.info(f"Getting tilt/position")
        self.tilt_cmd = command_interfaces["tilt/position"]

        self.logger.info(f"Getting pan/position")
        self.pan_cmd = command_interfaces["pan/position"]

        # Initialise value to starting angle
        self.tilt_cmd.value = self.start_tilt_angle
        self.pan_cmd.value = self.start_pan_angle

        # Add service
        self.service = self.node.create_service(ScimbalCamController.SERVICE_TYPE, self.service_name, self.on_request)

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    def on_request(self, request, response):
        """
        Called on every request. Applies displacement values of tilt and pan from request angles onto positional servos.

        :param request: Expects an array angles containing 2 angles displacing current tilt and pan [delta_tilt, delta_pan].
        :param response: Response to return after request has been processed
        :return: Response for request containing if the operation was a success or not.
        """
        try:
            # Extract variables from request
            delta_tilt: int = request.angles[0]
            delta_pan: int = request.angles[1]

            # Clamping displaced value
            self.tilt_cmd.value = max(self.min_tilt_angle, min(self.max_tilt_angle, self.tilt_cmd.value + delta_tilt))
            self.pan_cmd.value = max(self.min_pan_angle, min(self.max_pan_angle, self.pan_cmd.value + delta_pan))

            self.logger.info(f"Scimbal Cam angles updated: TILT: {self.tilt_cmd.value}, PAN: {self.pan_cmd.value}, request: {request.angles}")
            
            response.success = True

        except Exception as e:
            self.logger.error(f"Scimbal Cam angle update request {request.angles} interrupted by error: {str(e)}")
            
            response.success = False

        return response


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("scimbal_cam")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", ScimbalCamController) \
        .with_hardware("tilt", PositionalServoHardware, frame_id=0x0B0, function_id=0x03, min_angle=0.0, max_angle=180.0) \
        .with_hardware("pan", PositionalServoHardware, frame_id=0x0B0, function_id=0x04, min_angle=0.0, max_angle=360.0) \
        .with_jcan() \
        .spin()