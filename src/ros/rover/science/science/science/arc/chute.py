#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
System for the science chute which deposits sand 
into the kiln.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: PresetTwitchController
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - rotation/position    [value between 72 and 108]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Binuda Kalugalage
CREATION:       05/01/2026
EDITED:         26/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from python_control2 import PythonControl
from python_control2.controllers import PresetTwitchController
from python_control2.hardware_interfaces import PositionalServoHardware
from teleop_python_utils import Inputs


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("chute")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller(
            "controller", 
            PresetTwitchController, 
            max_angle=90.0,
            positions={
                "disengaged": 0.0,
                "engaged": 90.0
            },
            twitch_max=10.0,
            hardware_name="rotation"
        ) \
        .with_hardware("rotation", PositionalServoHardware, can_id=0x0E3) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()