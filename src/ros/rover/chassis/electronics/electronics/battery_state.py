#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
# from python_control.ControllerNode import ControllerNode
from sensor_msgs.msg import BatteryState
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
from python_control2.hardware_interfaces import CMDHardware
import time
import math
import jcan

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
<insert purpose here>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: Auto
TOPICS:
  - publisher: <topic> [<msg type>]
SERVICES:
	- service: <service> [<srv type>]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        <package>
AUTHOR(S):      <insert your name>
CREATION:       <current date>
EDITED:         <current date>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

class BatteryStateController(Controller):
    def __init__(self, contexts: Contexts):
        super().__init__(contexts)

        self.logger.info(f"Controller started.")

        self.publisher_battery_state = self.node.create_publisher(BatteryState, '/battery_state', 10)

        # time_interval= 5.0
        # self.update_timer = self.create_timer(time_interval, self.get_battery_state)

        self.logger.info("BatteryStateController started.")

    def on_configure(self, command_interfaces, state_interfaces):
        return True

    def on_update(self, now, period):
        msg = BatteryState()
        msg.voltage = 32
        msg.current = 2.1
        msg.percentage = 0.52
        msg.present = True

        self.publisher_battery_state.publish(msg)
        self.logger.info(f"battery is present: {msg.present}, published voltage: {msg.voltage}, current: {msg.current}, percentage: {msg.percentage}")


# def main(args=None):
#     rclpy.init()

if __name__ == '__main__':
    print("Setting up!")
    rclpy.init()

    PythonControl("battery_state", update_rate=5, can_bus="can0") \
        .with_controller("battery_controller", BatteryStateController) \
        .spin()
        # .with_jcan() \