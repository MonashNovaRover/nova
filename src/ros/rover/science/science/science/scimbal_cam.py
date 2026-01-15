#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Scimbal Cam movement with respective servos
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ScimbalCamNode
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
from teleop_python_utils import Inputs


class ScimbalCamNode(Controller):
    # Command interfaces
    # joint_cmd: Interface

    # State interfaces
    # state: Interface

    def __init__(self, contexts: Contexts, button: str="some_button", axis: str="some_axis"):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"ScimbalCamNode -- I have been __init__ialized")

        # Declare ROS2 parameters here.
        # self.joint = self.declare_parameter("joint", "j1").value

        # Do any setup logic here, save any contexts you want reference to in the future.
        # Save Input references here
        self.button_name = self.declare_parameter("button", button).value
        self.axis_name = self.declare_parameter("axis", axis).value

        inputs = contexts[Inputs]
        self.button = inputs.get_button(self.button_name)
        self.axis = inputs.get_axis(self.axis_name)
        inputs.get_event(f"{self.button_name}/down").add_callback(lambda : self.logger.info(f"{self.button_name}/down event triggered"))
        

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
        # self.logger.info(f"Getting \"{self.joint + "/effort"}\"")
        # self.joint_cmd = command_interfaces[self.joint + "/effort"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        # Update Command Interfaces
        # self.cmd.value = 2 * self.state.value
        # self.logger.info(f"{self.state.value} -> {self.cmd.value}")

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

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("control_test")
    inputs = Inputs(node).with_topics("/package/input")

    PythonControl("scimbal_cam", update_rate=5, can_bus="can1") \
        .with_controller("ScimbalCamNode", ScimbalCamNode) \
        .with_hardware("test_hw", TestHardware) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()