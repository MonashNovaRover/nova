#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controls Scraper X (2024-2025 scraper)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: scraper_x
TOPICS:
  - subscriber: /ec/input
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        excavation_construction
AUTHOR(S):      Jonathan Jia
CREATION:       17/01/2026
EDITED:         17/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface

from python_control2.hardware_interfaces import CMDHardware
from teleop_python_utils import Inputs


class ScraperXController(Controller):

    # Command interfaces
    arm_joint_cmd: Interface[float]
    scoop_joint_cmd: Interface[float]
    claw_joint_cmd: Interface[float]

    def __init__(self, contexts: Contexts,
                 arm_joint = "arm_joint",
                 arm_effort_multiplier = 1.0,
                 scoop_joint ="scoop_joint",
                 scoop_effort_multiplier = 1.0,
                 claw_joint ="claw_joint",
                 claw_effort_multiplier = 0.6,
                 minimum_speed = 0.05):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        # joint names (corresponds to command interfaces) and their effort multipliers
        self.arm_joint: str = self.declare_parameter("arm_joint", arm_joint).value
        self.arm_effort_multiplier: float = self.declare_parameter("arm_effort_multiplier", arm_effort_multiplier).value
        self.scoop_joint: str = self.declare_parameter("scoop_joint", scoop_joint).value
        self.scoop_effort_multiplier: float = self.declare_parameter("scoop_effort_multiplier", scoop_effort_multiplier).value
        self.claw_joint: str = self.declare_parameter("claw_joint", claw_joint).value
        self.claw_effort_multiplier: float = self.declare_parameter("claw_effort_multiplier", claw_effort_multiplier).value

        # axis/button names (corresponds to params passed to teleop modular)
        self.lock_button_name: str = self.declare_parameter("lock_button_name", "lock").value
        self.speed_axis_name: str = self.declare_parameter("speed_axis_name", "speed").value
        self.arm_axis_name: str = self.declare_parameter("arm_axis_name", "arm_joint").value
        self.scoop_axis_name: str = self.declare_parameter("scoop_axis_name", "scoop_joint").value
        self.claw_axis_name: str = self.declare_parameter("claw_axis_name", "claw_joint").value

        self.minimum_speed: float = self.declare_parameter("minimum_speed", minimum_speed).value

        inputs = contexts[Inputs]
        self.lock_button = inputs.get_button(self.lock_button_name)
        self.speed_axis = inputs.get_axis(self.speed_axis_name)
        self.arm_axis = inputs.get_axis(self.arm_axis_name)
        self.scoop_axis = inputs.get_axis(self.scoop_axis_name)
        self.claw_axis = inputs.get_axis(self.claw_axis_name)

        # inputs.get_event(f"{self.button_name}/down").add_callback(lambda : self.logger.info(f"{self.button_name}/down event triggered"))
        

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        self.arm_joint_cmd = command_interfaces[self.arm_joint + "/effort"]
        self.scoop_joint_cmd = command_interfaces[self.scoop_joint + "/effort"]
        self.claw_joint_cmd = command_interfaces[self.claw_joint + "/effort"]

        self.logger.info("ScraperXController configured")


    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """

        if self.lock_button:
            speed = 0.0
        else:
            speed = max(self.speed_axis.value, self.minimum_speed)

        self.arm_joint_cmd.value = self.arm_axis.value * speed
        self.scoop_joint_cmd.value = self.scoop_axis.value * speed
        self.claw_joint_cmd.value = self.claw_axis.value * speed

        self.logger.debug(f"speed: {speed}, arm effort: {self.arm_joint_cmd.value:.2f}, "
                          f"scoop effort: {self.scoop_joint_cmd.value:.2f}, "
                          f"claw effort: {self.claw_joint_cmd.value:.2f}")

def main():
    rclpy.init()

    node = Node("scraper_x")
    inputs = Inputs(node).with_topics("/ec/input")

    PythonControl(node, update_rate=20, can_bus="can1") \
        .with_controller("controller", ScraperXController,
                         arm_joint = "arm_joint",
                         scoop_joint ="scoop_joint",
                         claw_joint ="claw_joint") \
        .with_hardware("arm_actuator", CMDHardware, # TODO: replace with QCMD hardware interface
                       joint = "arm_joint",
                       can_id = 0x1) \
        .with_hardware("scoop_actuator", CMDHardware, # TODO: replace with QCMD hardware interface
                       joint = "scoop_joint",
                       can_id = 0x2) \
        .with_hardware("claw_actuator", CMDHardware, # TODO: replace with QCMD hardware interface
                       joint = "claw_joint",
                       can_id = 0x3) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()

if __name__ == "__main__":
    main()