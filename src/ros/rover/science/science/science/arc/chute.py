#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the science chute which 
deposits sand into the kiln.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ChuteController
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Binuda Kalugalage
CREATION:       05/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, Activation
from python_control2.hardware_interfaces import PositionalServoHardware
from teleop_python_utils import Inputs


class ChuteController(Controller):
    # Command interfaces
    pos_cmd: Interface

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"ChuteController -- I have been __init__ialized")

        self.active = contexts[Activation]

        # Declare ROS2 parameters here.
        self.offset_step_max = self.declare_parameter("offset_step_max", 30.0).value
        self.disengaged_pos = self.declare_parameter("disengaged_pos", 72.0).value
        self.engaged_pos = self.declare_parameter("engaged_pos", 108.0).value

        # Do any setup logic here, save any contexts you want reference to in the future.
        # Save Input references here
        self.speed_axis_name = self.declare_parameter("speed_axis", "chute_speed").value

        self.button_engaged_name = self.declare_parameter("engaged_button", "chute_engaged").value
        self.button_disengaged_name = self.declare_parameter("disengaged_button", "chute_disengaged").value
        self.button_twitch_increase_name = self.declare_parameter("plus_button", "chute_twitch_increase").value
        self.button_twitch_decrease_name = self.declare_parameter("minus_button", "chute_twitch_decrease").value

        inputs = contexts[Inputs]
        self.speed_axis = inputs.get_axis(self.speed_axis_name)

        self.button_engaged = inputs.get_button(self.button_engaged_name)
        self.button_disengaged = inputs.get_button(self.button_disengaged_name)
        self.button_twitch_increase = inputs.get_button(self.button_twitch_increase_name)
        self.button_twitch_decrease = inputs.get_button(self.button_twitch_decrease_name)

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
        self.logger.info(f"Getting rotation/position")
        self.pos_cmd = command_interfaces["rotation/position"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        if not self.active:
            return

        # Update offset step amount
        offset_step = abs(self.speed_axis.value) * self.offset_step_max

        # Change to preset position
        if self.button_engaged.down():
            self.offset = 0.0
            self.current_pos = self.engaged_pos
            self.logger.info(f"Moved to ENGAGED position {self.current_pos}")
        elif self.button_disengaged.down():
            self.offset = 0.0
            self.current_pos = self.disengaged_pos
            self.logger.info(f"Moved to DISENGAGED position {self.current_pos}")

        # Twitch/update offset
        if self.button_twitch_increase:
            self.offset += offset_step
        elif self.button_twitch_decrease:
            self.offset -= offset_step

        # Write to command interface
        self.pos_cmd.value = self.current_pos + self.offset

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("chute")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", ChuteController) \
        .with_hardware("rotation", PositionalServoHardware, function_id=0x03, min_angle=72.0, max_angle=108.0) \
        .with_teleop(inputs) \
        .with_activation_buttons(
            active_button_name="activate_chute",
            inactive_button_pool_names=[
                "activate_auger",
                "activate_sweeper",
            ],
        ) \
        .with_jcan() \
        .spin()