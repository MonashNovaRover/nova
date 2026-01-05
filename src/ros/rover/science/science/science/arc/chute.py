#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control for the chute
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ChuteController
TOPICS:   None
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Binuda Kalugalage
CREATION:       05/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
import jcan
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface

from python_control2.hardware_interfaces import PositionalServoHardware
from teleop_python_utils import Inputs


class ChuteController(Controller):
    # Command interfaces
    pos_cmd: Interface

    # State interfaces
    # state: Interface

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"ChuteController -- I have been __init__ialized")

        self.min_angle = 72.0
        self.max_angle = 108.0

        # Declare ROS2 parameters here.
        self.offset_step_max = float(self.declare_parameter("offset_step_max", 30.0).value)
        self.disengaged_pos = float(self.declare_parameter("disengaged_pos", self.min_angle).value)
        self.engaged_pos = float(self.declare_parameter("engaged_pos", self.max_angle).value)

        # Do any setup logic here, save any contexts you want reference to in the future.
        # Save Input references here
        self.speed_axis_name = self.declare_parameter("speed_axis", "chute_speed").value

        self.button_engaged_name = self.declare_parameter("engaged_button", "chute_engaged").value
        self.button_disengaged_name = self.declare_parameter("disengaged_button", "chute_disengaged").value
        self.button_plus_name = self.declare_parameter("plus_button", "chute_plus").value
        self.button_minus_name = self.declare_parameter("minus_button", "chute_minus").value

        inputs = contexts[Inputs]
        self.speed_axis = inputs.get_axis(self.speed_axis_name)

        self.button_engaged = inputs.get_button(self.button_engaged_name)
        self.button_disengaged = inputs.get_button(self.button_disengaged_name)
        self.button_plus = inputs.get_button(self.button_plus_name)
        self.button_minus = inputs.get_button(self.button_minus_name)

        self.current_pos = self.disengaged_pos
        self.offset = 0.0

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
        self.logger.info(f"Getting chute_hw/position")
        self.pos_cmd = command_interfaces["chute_hw/position"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """

        # Update offset step amount
        offset_step = float(abs(self.speed_axis.value) * self.offset_step_max)

        # Change to preset position
        if self.button_engaged:
            self.offset = 0.0
            self.current_pos = self.engaged_pos
            self.logger.info(f"Moved to ENGAGED position {self.current_pos}")
        elif self.button_disengaged:
            self.offset = 0.0
            self.current_pos = self.disengaged_pos
            self.logger.info(f"Moved to DISENGAGED position {self.current_pos}")

        # Twitch/update offset
        if self.button_plus:
            self.offset += offset_step
        elif self.button_minus:
            self.offset -= offset_step

        # Clamp position to between 72-108 and write to command interface
        clamped_position = max(self.min_angle, min(self.max_angle, self.current_pos + self.offset))
        self.pos_cmd.value = float(clamped_position)

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("chute")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("chute_controller", ChuteController) \
        .with_hardware("chute_hw", PositionalServoHardware, function_id=0x03, min_angle=72.0, max_angle=108.0) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()