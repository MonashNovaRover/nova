#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
<insert purpose here>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: AugerController
TOPICS:
  - publisher: <topic> [<msg type>]
SERVICES:
	- service: <service> [<srv type>]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      <insert your name>
CREATION:       <current date>
EDITED:         <current date>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
import jcan
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface

from python_control2.hardware_interfaces import CMDHardware
from teleop_python_utils import Inputs


class AugerController(Controller):
    # Command interfaces
    # joint_cmd: Interface

    # State interfaces
    # state: Interface

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"AugerController -- I have been __init__ialized")

        # Declare ROS2 parameters here.
        # self.joint = self.declare_parameter("joint", "j1").value

        # Do any setup logic here, save any contexts you want reference to in the future.
        # self.button = contexts[Inputs].get_button("some_button")
        # self.axis = contexts[Inputs].get_axis("some_axis")
        # contexts[Inputs].get_event("some_button/down", f"some_button was just pressed down")
        

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param command_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
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

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    PythonControl("auger", update_rate=5, can_bus="can1") \
        .with_controller("AugerController", AugerController) \
        .with_hardware("test_hw", TestHardware) \
        .with_jcan() \
        .with_teleop(lambda node: Inputs(node).with_topic("science/inputs")) \
        .spin()