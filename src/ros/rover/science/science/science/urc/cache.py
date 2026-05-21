#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controls the URC Caches (positional servos)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: PresetTwitchController
COMMAND INTERFACES:
  - actuation/position   [value between 0 and 180]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Binuda Kalugalage
CREATION:       17/2/2026
EDITED:         15/5/2026
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

    node = Node("cache")
    node_name = node.get_name()
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller(
            "controller",
            PresetTwitchController,
            min_angle=0.0,
            max_angle=180.0,
            initial_angle=0.0,
            positions={
                "closed": 0.0,
                "half_open": 90.0,
                "fully_open": 180.0,
            },
            twitch_max=30.0,
            hardware_name="actuation",
            set_position_service=f"/science/{node_name}/set_position",
            twitch_service=f"/science/{node_name}/twitch",
            position_topic=f"/science/{node_name}/position"
        ) \
        .with_hardware("actuation", PositionalServoHardware, can_id=0x0E2) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()
