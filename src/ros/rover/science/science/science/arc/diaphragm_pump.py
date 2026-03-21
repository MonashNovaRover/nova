#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
System for the science Diaphragm Pump which
creates a movement of air through the condenser.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - flow/effort    [value between 0 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Binuda Kalugalage
CREATION:       01/02/2026
EDITED:         05/03/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import jcan

import rclpy
from rclpy.node import Node
from python_control2 import PythonControl
from python_control2.controllers import EffortCommandController
from python_control2.hardware_interfaces import QCMDHardware
from teleop_python_utils import Inputs

if __name__ == "__main__":
    print("Setting up!") 

    rclpy.init()

    node = Node("diaphragm_pump")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller(
            "controller",
            EffortCommandController,
            hardware_name="flow",
            service_name="/science/diaphragm_pump_command",
            topic_name="/science/diaphragm_pump_status"
        ) \
        .with_hardware("flow", QCMDHardware, can_id=0x0C2) \
        .with_jcan() \
        .spin()