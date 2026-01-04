#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control for the Spinny Part of the ARC
analysis arm
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: SpinnyPartController
TOPICS:   None
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Binuda Kalugalage
CREATION:       03/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
import jcan
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface

from python_control2.hardware_interfaces import PositionalServoHardware
from teleop_python_utils import Inputs


class SpinnyPartController(Controller):
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
        self.logger.info(f"SpinnyPartController -- I have been __init__ialized")

        self.min_angle = 0.0
        self.max_angle = 180.0

        # Declare ROS2 parameters here.
        self.offset_step_max = float(self.declare_parameter("offset_step_max", 30.0).value)
        self.sweeper_pos = float(self.declare_parameter("sweeper_pos", 0.0).value)
        self.microscope_pos = float(self.declare_parameter("microscope_pos", (128.0 / 255.0) * self.max_angle).value)
        self.nir_probe_pos = float(self.declare_parameter("nir_probe_pos", self.max_angle).value)

        # Do any setup logic here, save any contexts you want reference to in the future.
        # Save Input references here
        self.speed_axis_name = self.declare_parameter("speed_axis", "spinny_speed").value

        self.button_sweeper_name = self.declare_parameter("sweeper_button", "spinny_sweeper").value
        self.button_microscope_name = self.declare_parameter("microscope_button", "spinny_microscope").value
        self.button_nir_name = self.declare_parameter("nir_button", "spinny_nir").value
        self.button_plus_name = self.declare_parameter("plus_button", "spinny_plus").value
        self.button_minus_name = self.declare_parameter("minus_button", "spinny_minus").value

        inputs = contexts[Inputs]
        self.speed_axis = inputs.get_axis(self.speed_axis_name)

        self.button_sweeper = inputs.get_button(self.button_sweeper_name)
        self.button_microscope = inputs.get_button(self.button_microscope_name)
        self.button_nir = inputs.get_button(self.button_nir_name)
        self.button_plus = inputs.get_button(self.button_plus_name)
        self.button_minus = inputs.get_button(self.button_minus_name)

        self.current_pos = self.nir_probe_pos
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
        self.logger.info(f"Getting spinny_hw/position")
        self.pos_cmd = command_interfaces["spinny_hw/position"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """

        # Update offset step amount
        offset_step = float(abs(self.speed_axis.value) * self.offset_step_max)

        # Change to preset position
        if self.button_sweeper:
            self.offset = 0.0
            self.current_pos = self.sweeper_pos
            self.logger.info(f"Moved to SWEEPER position {self.current_pos}")
        elif self.button_microscope:
            self.offset = 0.0
            self.current_pos = self.microscope_pos
            self.logger.info(f"Moved to MICROSCOPE position {self.current_pos}")
        elif self.button_nir:
            self.offset = 0.0
            self.current_pos = self.nir_probe_pos
            self.logger.info(f"Moved to NIR PROBE position {self.current_pos}")

        # Twitch/update offset
        if self.button_plus:
            self.offset += offset_step
        elif self.button_minus:
            self.offset -= offset_step

        # Clamp position to between 0-180 and write to command interface
        clamped_position = max(self.min_angle, min(self.max_angle, self.current_pos + self.offset))
        self.pos_cmd.value = float(clamped_position)


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("spinny_part")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("spinny_controller", SpinnyPartController) \
        .with_hardware("spinny_hw", PositionalServoHardware, function_id=0x04, min_angle=0.0, max_angle=180.0) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()