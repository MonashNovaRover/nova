#!/usr/bin/env python3
import rclpy
from rclpy.node import Node, ParameterDescriptor
# from python_control.ControllerNode import ControllerNode
from sensor_msgs.msg import BatteryState
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
from python_control2.hardware_interfaces import CMDHardware
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
to run with parameter file:
$ ros2 run electronics battery_state.py --ros-args --params-file ~/nova/src/ros/rover/nova_bringup/params/battery_state.yaml
testing ws (del later):
$ ~/Builds/testing/bin/ros2 run electronics battery_state.py --ros-args --params-file ~/nova/src/ro
s/rover/nova_bringup/params/battery_state.yaml
"""

class BatteryStateController(Controller):
    def __init__(self, contexts: Contexts):
        super().__init__(contexts)

        self.publisher_battery_state = self.node.create_publisher(BatteryState, '/battery_state', 10)

        # time_interval= 5.0
        # self.update_timer = self.create_timer(time_interval, self.get_battery_state)
        self.parsed_voltage = float('nan')
        self.parsed_current = float ('nan')
        self.logger.info("BatteryStateController started.")

    def on_configure(self, command_interfaces, state_interfaces):
        return True

    def on_update(self, now, period):
        msg = BatteryState()

        if math.isnan(self.parsed_voltage):
            msg.present = False 
            msg.voltage = float('nan')
            msg.current = float('nan')

        msg.voltage = self.parsed_voltage
        msg.present = True
        msg.current = self.parsed_current

        self.publisher_battery_state.publish(msg)
        self.logger.info(f"battery is present: {msg.present}, published voltage: {msg.voltage}, current: {msg.current}")

class BatteryStateHardware(HardwareInterface):

    VOLTAGE_CURRENT_CANID = 0x4B2

    def __init__(self, contexts: Contexts):
        super().__init__(contexts)
        self.logger.info(f"Hardware interface initialised")
        self.bus = contexts[jcan.Bus]

        self.voltage = float('nan')
        self.current = float('nan')

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        # self.state = state_interfaces["state"]
        self.logger.info(f"Checking {self.VOLTAGE_CURRENT_CANID}")
        self.bus.add_callback(self.VOLTAGE_CURRENT_CANID, self.get_battery_frame)

    def on_read(self, now, period):
        pass

    def get_battery_frame(self, frame):
        self.logger.info(f"Received data: {frame.data} from: {frame.id}")
        current = int.from_bytes(frame.data[0:2], 'big', signed=True) 
        voltage = int.from_bytes(frame.data[2:4], 'big', signed=False) 

        self.parsed_current = current/100.0
        self.parsed_voltage = voltage/1000.0
        self.logger.info(f"parsed voltage: {self.parsed_voltage}. current: {self.parsed_current}")

    def on_write(self, now, period):
        pass

if __name__ == '__main__':
    print("Setting up!")
    rclpy.init()

    PythonControl("battery_state", update_rate=5, can_bus="can0") \
        .with_hardware("battery_hw", BatteryStateHardware) \
        .with_controller("battery_controller", BatteryStateController) \
        .with_jcan() \
        .spin()