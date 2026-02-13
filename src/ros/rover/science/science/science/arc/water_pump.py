#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
System for the science Water Pump which cools the
condenser.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - cooling/effort    [value between 0 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Binuda Kalugalage
CREATION:       01/02/2026
EDITED:         01/02/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from python_control2 import PythonControl
from python_control2.controllers import EffortCommandController
from python_control2.hardware_interfaces import QCMDHardware

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("water_pump")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller(
            "controller",
            EffortCommandController,
            hardware_name="cooling",
            service_name="/science/water_pump_command",
            topic_name="/science/water_pump_status"
        ) \
        .with_hardware("cooling", QCMDHardware, can_id=0xD2) \
        .with_jcan() \
        .spin()