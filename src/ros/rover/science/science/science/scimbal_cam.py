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

from enum import Enum
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
from python_control2.hardware_interfaces import CMDHardware


class ScimbalCamController(Controller):
    # Command interfaces
    tilt_cmd: Interface
    pan_cmd: Interface

    # State interfaces
    # state: Interface

    def __init__(self, contexts: Contexts, button: str="some_button", axis: str="some_axis"):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"ScimbalCamNode -- I have been __init__ialized")

        self.start_tilt_angle = 90
        self.start_pan_angle = 180

        self.max_tilt_angle = 180
        self.max_pan_angle = 360

        # Declare ROS2 parameters here.
        # self.joint = self.declare_parameter("joint", "j1").value
        

        # Do any setup logic here, save any contexts you want reference to in the future.
        # Save Input references here
        
        

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
        self.logger.info(f"Getting tilt_hw/position")
        self.tilt_cmd = command_interfaces["tilt_hw/position"]

        self.logger.info(f"Getting pan_hw/position")
        self.pan_cmd = command_interfaces["pan_hw/position"]


    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    def on_request(self, request, response):
        try:
            delta_tilt, delta_pan = request.angles[0], request.angles[1]

            

            self.get_logger().info(f"Scimbal Cam angles updated: TILT: {self.scimbal_cam[0].get_goal_position()}, PAN: {self.scimbal_cam[1].get_goal_position()}, request: {request.angles}")
            response.success = True
        except Exception as e:
            self.get_logger().error(f"Scimbal Cam angle update request {request.angles} interrupted by error: {str(e)}")
            response.success = False
        return response

    # OLD displace angle function for reference
    # 
    # def request_servo(self, request, response):
    #     try:
    #         for i in range(len(request.angles)):
    #             angle = request.angles[i]
    #             self.scimbal_cam[i].displace(angle)
    #         self.get_logger().info(f"Scimbal Cam angles updated: TILT: {self.scimbal_cam[0].get_goal_position()}, PAN: {self.scimbal_cam[1].get_goal_position()}, request: {request.angles}")
    #         response.success = True
    #     except Exception as e:
    #         self.get_logger().error(f"Scimbal Cam angle update request {request.angles} interrupted by error: {str(e)}")
    #         response.success = False
    #     return response

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("scimbal_cam")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", ScimbalCamController) \
        .with_hardware("tilt_hw", PositionalServoHardware, frame_id=0x0B0, function_id=0x03, max_angle=180.0) \
        .with_hardware("pan_hw", PositionalServoHardware, frame_id=0x0B0, function_id=0x04, max_angle=360.0) \
        .with_jcan() \
        .spin()