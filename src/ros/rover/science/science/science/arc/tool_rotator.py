#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
System for the Tool Rotator of the ARC
analysis arm which switches between instruments.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: PresetTwitchController
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - rotation/position    [value between 0 and 180]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Binuda Kalugalage
CREATION:       03/01/2026
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

    node = Node("tool_rotator")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller(
            "controller", 
            PresetTwitchController, 
            max_angle=168.71,
            positions={
                "sweeper": 0.0,
                "microscope": 169.41,
                "nir_probe": 337.41,
            },
            twitch_max=30.0,
            hardware_name="rotation"
        ) \
        .with_hardware("rotation", PositionalServoHardware, function_id=0x00, angular_limit=360.0) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()