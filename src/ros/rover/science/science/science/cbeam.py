#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the science CBeam which actuates up
and down.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - actuation/effort    [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       13/01/26
EDITED:         13/01/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from python_control2 import PythonControl, ActuateController
from python_control2.hardware_interfaces import QCMDHardware
from teleop_python_utils import Inputs

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("cbeam")
    inputs = Inputs(node).with_topics("/science/input")

    # ARCh auger system
    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller(
            "controller",
            ActuateController,
            hardware_name="actuation",
            actuation_axis="auger_actuation"
        ) \
        .with_hardware("actuation", QCMDHardware, can_id=0xC2) \
        .with_teleop(inputs) \
        .with_activation_buttons(start_active=False, active_button_name="activate_cbeam", inactive_button_pool_names=["activate_auger"]) \
        .with_jcan() \
        .spin()