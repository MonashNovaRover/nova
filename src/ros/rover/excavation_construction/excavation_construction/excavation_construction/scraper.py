#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controls the scraper (arm, scoop and claw)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: scraper
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - arm/effort      [value between -1 and 1]
  - scoop/effort    [value between -1 and 1]
  - claw/effort     [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        excavation_construction
AUTHOR(S):      Jonathan Jia
CREATION:       17/01/2026
EDITED:         25/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface

from python_control2.hardware_interfaces import QCMDHardware
from teleop_python_utils import Inputs


class ScraperController(Controller):

    # Command interfaces
    arm_joint_cmd: Interface[float]
    scoop_joint_cmd: Interface[float]
    claw_joint_cmd: Interface[float]

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        self.minimum_speed: float = self.declare_parameter("minimum_speed", 0.05,
                                                           "minimum scraper actuation speed (if unlocked)").value

        self.speed_axis_name: str = self.declare_parameter("speed_axis_name", "scraper_speed").value
        self.arm_axis_name: str = self.declare_parameter("arm_axis_name", "arm_actuation").value
        self.scoop_axis_name: str = self.declare_parameter("scoop_axis_name", "scoop_actuation").value
        self.claw_axis_name: str = self.declare_parameter("claw_axis_name", "claw_actuation").value

        inputs = contexts[Inputs]
        self.speed_axis = inputs.get_axis(self.speed_axis_name)
        self.arm_axis = inputs.get_axis(self.arm_axis_name)
        self.scoop_axis = inputs.get_axis(self.scoop_axis_name)
        self.claw_axis = inputs.get_axis(self.claw_axis_name)

        self.logger.info(f"ScraperController initialised with minimum speed: {self.minimum_speed:.2f}")

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        self.logger.info(f"Getting arm/effort, scoop/effort and claw/effort command interfaces")

        self.arm_joint_cmd = command_interfaces["arm/effort"]
        self.scoop_joint_cmd = command_interfaces["scoop/effort"]
        self.claw_joint_cmd = command_interfaces["claw/effort"]

        self.logger.info("ScraperController configured")


    def on_update(self, now: float, period: float):
        """ Called on every update.

        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        speed = max(self.speed_axis.value, self.minimum_speed)

        self.arm_joint_cmd.value = self.arm_axis.value * speed
        self.scoop_joint_cmd.value = self.scoop_axis.value * speed
        self.claw_joint_cmd.value = self.claw_axis.value * speed

        self.logger.debug(f"speed: {speed}, arm effort: {self.arm_joint_cmd.value:.2f}, "
                          f"scoop effort: {self.scoop_joint_cmd.value:.2f}, "
                          f"claw effort: {self.claw_joint_cmd.value:.2f}")

def main():
    rclpy.init()

    node = Node("scraper")
    inputs = Inputs(node).with_topics("/ec/input")

    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("controller", ScraperController) \
        .with_hardware("arm", QCMDHardware) \
        .with_hardware("scoop", QCMDHardware) \
        .with_hardware("claw", QCMDHardware) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()

if __name__ == "__main__":
    main()