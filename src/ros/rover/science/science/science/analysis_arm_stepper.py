#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the science analysis arm which
actuates up and down.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - actuation/effort      [value between -1 and 1]
  - actuation/position    [number of steps away from zero position]
STATE INTERFACES:
  - actuation/position    [number of steps away from zero position]
  - tof/distance          [distance from ground in ??]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       01/02/26
EDITED:         04/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional

from python_control2 import PythonControl, Controller, Interface, Contexts, InterfaceCollection
from python_control2.hardware_interfaces import StepperHardware
from teleop_python_utils import Inputs, EventCollection


class AnalysisArmController(Controller):
    # Command interfaces
    actuation_effort_cmd: Interface
    actuation_position_cmd: Interface
    actuation_position_state: Interface

    def __init__(self, contexts: Contexts, hardware_name: str="actuation", actuation_axis: str="actuation"):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"AnalysisArmController -- I have been __init__ialized")

        # Get inputs
        self.hardware_name = self.declare_parameter("hardware_name", hardware_name).value

        # Actuation axis
        self.actuation_axis_name = self.declare_parameter("actuation_axis", actuation_axis).value

        # Get inputs
        inputs = contexts[Inputs]
        self.actuation_axis = inputs.get_axis(self.actuation_axis_name)

        # Get stepper zero event
        if EventCollection in contexts:
            events = contexts[EventCollection]
            self.zero_event = events.get(f"{self.hardware_name}/zero")
        else:
            self.logger.error("Could not find EventCollection in the python control contexts, cannot zero analysis arm position.")


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
        self.logger.info(f"Getting {self.hardware_name}/effort")
        self.actuation_cmd = command_interfaces[f"{self.hardware_name}/effort"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        if not self.active:
            self.actuation_cmd.value = 0
            return

        # Update actuation
        self.actuation_cmd.value = self.actuation_axis.value


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("analysis_arm")
    inputs = Inputs(node).with_topics("/science/input")

    # ARCh analysis arm system with stepper
    PythonControl(node, update_rate=2, can_bus="can1") \
        .with_controller(
        "controller",
        AnalysisArmController,
        actuation_axis="analysis_arm_actuation"
    ) \
        .with_hardware("actuation", StepperHardware, can_id=0x0E4) \
        .with_teleop(inputs) \
        .with_jcan() \
        .with_event_collection() \
        .spin()