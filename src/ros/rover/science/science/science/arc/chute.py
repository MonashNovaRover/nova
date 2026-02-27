#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
System for the science chute which deposits 
collected sand by rotating to face the kiln.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - rotation/position    [value between 72 and 108]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Binuda Kalugalage
CREATION:       05/01/2026
EDITED:         22/02/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from python_control2 import PythonControl
from python_control2.controllers import ActuateController
from python_control2.hardware_interfaces import ContinousServoHardware
from teleop_python_utils import Inputs


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("chute")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller(
            "controller", 
            ActuateController,
            hardware_name="rotation",
            actuation_axis="chute_actuation",
            speed_axis="chute_speed"
        ) \
        .with_hardware("rotation", ContinousServoHardware, can_id=0x0E2) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()