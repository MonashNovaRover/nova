#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the Tool Rotator of the ARC
analysis arm which switches between instruments.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ToolRotatorController
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - rotation/position    [value between 0 and 180]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Binuda Kalugalage
CREATION:       03/01/2026
EDITED:         24/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface
from python_control2.hardware_interfaces import PositionalServoHardware
from teleop_python_utils import Inputs


class ToolRotatorController(Controller):
    # Command interfaces
    pos_cmd: Interface

    def __init__(self, contexts: Contexts,
                 twitch_max: float = 30.0,
                 sweeper_pos: float = 0.0,
                 microscope_pos: float = 90.35,
                 nir_probe_pos: float = 168.71):
        """ Constructor for ToolRotatorController

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param twitch_max: Maximum position the tool rotator will twitch, in degrees.
        :param sweeper_pos: Position of the sweeper, in degrees.
        :param microscope_pos: Position of the microscope, in degrees.
        :param nir_probe_pos: Position of the NIR probe, in degrees.
        """
        super().__init__(contexts)
        self.logger.info(f"ToolRotatorController -- I have been __init__ialized") 

        self.twitch_max: float = self.declare_parameter("twitch_max", twitch_max).value
        self.sweeper_pos: float = self.declare_parameter("sweeper_pos", sweeper_pos).value
        self.microscope_pos: float = self.declare_parameter("microscope_pos", microscope_pos).value
        self.nir_probe_pos: float = self.declare_parameter("nir_probe_pos", nir_probe_pos).value

        # Get inputs
        self.speed_axis_name = self.declare_parameter("speed_axis", "rotator_speed").value

        self.button_sweeper_name = self.declare_parameter("sweeper_button", "rotator_sweeper").value
        self.button_microscope_name = self.declare_parameter("microscope_button", "rotator_microscope").value
        self.button_nir_name = self.declare_parameter("nir_button", "rotator_nir").value
        self.button_twitch_increase_name = self.declare_parameter("plus_button", "rotator_twitch_increase").value
        self.button_twitch_decrease_name = self.declare_parameter("minus_button", "rotator_twitch_decrease").value

        inputs = contexts[Inputs]
        self.speed_axis = inputs.get_axis(self.speed_axis_name)

        self.button_sweeper = inputs.get_button(self.button_sweeper_name)
        self.button_microscope = inputs.get_button(self.button_microscope_name)
        self.button_nir = inputs.get_button(self.button_nir_name)
        self.button_twitch_increase = inputs.get_button(self.button_twitch_increase_name)
        self.button_twitch_decrease = inputs.get_button(self.button_twitch_decrease_name)

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
        self.logger.info(f"Getting rotation/position")
        self.pos_cmd = command_interfaces["rotation/position"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """

        # Update twitch amount
        twitch_step = abs(self.get_rotator_speed()) * self.twitch_max

        # Change to preset position
        if self.button_sweeper.down():
            self.offset = 0.0
            self.current_pos = self.sweeper_pos
            self.logger.info(f"Moved to SWEEPER position {self.current_pos}")
        elif self.button_microscope.down():
            self.offset = 0.0
            self.current_pos = self.microscope_pos
            self.logger.info(f"Moved to MICROSCOPE position {self.current_pos}")
        elif self.button_nir.down():
            self.offset = 0.0
            self.current_pos = self.nir_probe_pos
            self.logger.info(f"Moved to NIR PROBE position {self.current_pos}")

        # Twitch/update offset
        if self.button_twitch_increase:
            self.offset += twitch_step
        elif self.button_twitch_decrease:
            self.offset -= twitch_step

        # Write to command interface
        self.pos_cmd.value = self.current_pos + self.offset
    
    def get_rotator_speed(self) -> float:
        """ Gets the tool rotator speed, mapping an axis [-1, 1] to a speed [0, 1]"""
        return (self.speed_axis.value + 1) / 2
    

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("tool_rotator")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", ToolRotatorController) \
        .with_hardware("rotation", PositionalServoHardware, function_id=0x00) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()