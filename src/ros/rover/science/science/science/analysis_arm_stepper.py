#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the science analysis arm which
actuates up and down.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - actuation/effort    [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       01/02/26
EDITED:         01/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from python_control2 import PythonControl, ActuateController
from python_control2.hardware_interfaces import StepperHardware
from teleop_python_utils import Inputs

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("analysis_arm")
    inputs = Inputs(node).with_topics("/science/input")

    # ARCh analysis arm system with stepper
    PythonControl(node, update_rate=2, can_bus="can1") \
        .with_controller(
        "controller",
        ActuateController,
        hardware_name="actuation",
        actuation_axis="analysis_arm_actuation"
    ) \
        .with_hardware("actuation", StepperHardware, can_id=0x0E6) \
        .with_teleop(inputs) \
        .with_activation_buttons(start_active=True, active_button_name="na", inactive_button_pool_names=["na"]) \
        .with_jcan() \
        .spin()