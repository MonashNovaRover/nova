#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the science sweeper which spins.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: Sweeper
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - sweep/effort    [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Brandon Chung
CREATION:       05/02/26
EDITED:         17/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, Direction, Activation
from python_control2.hardware_interfaces import QCMDHardware
from teleop_python_utils import Inputs


class SweeperController(Controller):
    # Command interfaces
    sweep_cmd: Interface

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"SweeperController -- I have been __init__ialized")

        self.active = contexts[Activation]

        # Get inputs
        inputs = contexts[Inputs]

        # Sweeper actuation
        self.sweeper_axis_name = self.declare_parameter("sweeper_axis", "sweeper_actuation").value
        self.sweeper_axis = inputs.get_axis(self.sweeper_axis_name)

        # Sweeper speed
        self.speed_axis_name = self.declare_parameter("speed_axis", "sweeper_speed").value
        self.speed_axis = inputs.get_axis(self.speed_axis)

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
        self.logger.info(f"Getting sweep/effort")
        self.sweep_cmd = command_interfaces["sweep/effort"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """

        # Update Command Interfaces

        # Set new command for sweep
        self.sweep_cmd.value = self.sweeper_axis.value * self.speed_axis.value

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("sweeper")
    inputs = Inputs(node).with_topics("/science/input")

    # ARCh auger system
    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("controller", SweeperController) \
        .with_hardware("sweep", QCMDHardware, can_id=0x0E4) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()
