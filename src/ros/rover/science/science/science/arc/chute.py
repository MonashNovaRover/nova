#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the science chute which 
deposits sand into the kiln.
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
            min_angle=72.0, 
            max_angle=108.0,
            positions={
            "disengaged": 72.0,
            "engaged": 108.0
            },
            twitch_max=6.0
        ) \
        .with_hardware("rotation", PositionalServoHardware, function_id=0x03) \
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