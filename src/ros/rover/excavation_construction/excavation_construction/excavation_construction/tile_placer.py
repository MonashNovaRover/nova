#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the tile placer payload which
moves the forklift up or down
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: tile_placer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - forklift/effort     [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        excavation_construction
AUTHOR(S):      Jonathan Jia
CREATION:       25/01/26
EDITED:         25/01/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface

from python_control2.hardware_interfaces import CMDHardware
from teleop_python_utils import Inputs

class TilePlacerController(Controller):

    # Command interfaces
    forklift_cmd: Interface[float]

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        self.minimum_speed: float = self.declare_parameter("minimum_speed", 0.05, "minimum forklift speed (if unlocked)").value
        self.forklift_effort_multiplier: float = self.declare_parameter("forklift_effort_multiplier", 0.70,
                                                                        "multiplies effort requested from the forklift actuator (is also max effort)").value

        self.speed_axis_name: str = self.declare_parameter("speed_axis_name", "forklift_speed").value
        self.forklift_actuation_axis_name: str = self.declare_parameter("forklift_actuation_axis_name", "forklift_actuation").value

        inputs = contexts[Inputs]
        self.speed_axis = inputs.get_axis(self.speed_axis_name)
        self.forklift_actuation_axis = inputs.get_axis(self.forklift_actuation_axis_name)

        self.logger.info(f"TilePlacerController initialised with minimum speed: {self.minimum_speed:.2f} "
                         f"and forklift effort multiplier: {self.forklift_effort_multiplier:.2f}")

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        self.logger.info(f"Getting forklift/effort command interface")
        self.forklift_cmd = command_interfaces["forklift/effort"]

        self.logger.info(f"TilePlacerController configured")


    def on_update(self, now: float, period: float):
        """ Called on every update.

        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        speed = max(self.speed_axis.value, self.minimum_speed)

        self.forklift_cmd.value = self.forklift_actuation_axis.value * self.forklift_effort_multiplier * speed

        self.logger.debug(f"speed: {speed}, forklift effort: {self.forklift_cmd.value:.2f}")

def main():
    rclpy.init()

    node = Node("tile_placer")
    inputs = Inputs(node).with_topics("/ec/input")

    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("controller", TilePlacerController) \
        .with_hardware("forklift", CMDHardware, # TODO: replace with QCMD hardware interface
                       can_id = 0x1) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()

if __name__ == "__main__":
    main()