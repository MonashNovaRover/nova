#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Track voltage and current status for ARCh power usage task
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: battery_state
TOPICS:
  - publisher: /battery_state [sensor_msgs/msg/BatteryState]
SERVICES:
	- service: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        electronics
AUTHOR(S):      Danielle Rosenthal
CREATION:       11/12/2025
EDITED:         11/12/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
to run with parameter file:
$ ~/Builds/<build name>/bin/ros2 run electronics battery_state.py --ros-args --params-file ~/nova/src/ros/rover/nova_bringup/params/battery_state.yaml
"""

import rclpy
from rclpy.node import Node, ParameterDescriptor
from sensor_msgs.msg import BatteryState
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
from python_control2.hardware_interfaces import CMDHardware
import math
import jcan

class BatteryStateController(Controller):
    def __init__(self, contexts: Contexts):
        super().__init__(contexts)

        self.publisher_battery_state = self.node.create_publisher(BatteryState, '/battery_state', 10)

        self.voltage_state = None
        self.current_state = None
        self.logger.info("BatteryStateController started.")

    def on_configure(self, command_interfaces, state_interfaces):
        self.voltage_state = state_interfaces["battery/voltage"]
        self.current_state = state_interfaces["battery/current"]
        self.logger.info("Getting state of current and voltage")
        return True

    def on_update(self, now, period):
        """ 
        Purpose: Publishes most recent battery voltage and current to /battery_state of type sensor_msg.msg/BatteryState
        msg type:
        https://docs.ros.org/en/jade/api/sensor_msgs/html/msg/BatteryState.html
        """
        #BatteryState here is the ROS2 publisher node
        msg = BatteryState()

        if math.isnan(self.voltage_state.value):
            msg.voltage = float('nan')
            msg.current = float('nan')

        msg.voltage = self.voltage_state.value
        msg.current = self.current_state.value

        self.publisher_battery_state.publish(msg)

class BatteryStateHardware(HardwareInterface):

    def __init__(self, contexts: Contexts):
        super().__init__(contexts)
        self.logger.info(f"Hardware interface initialised")
        self.bus = contexts[jcan.Bus]

        self.voltage = float('nan')
        self.current = float('nan')
        self.declare_parameter("VOLTAGE_CURRENT_CANID", 0x4B2)

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        can_id = self.get_parameter("VOLTAGE_CURRENT_CANID").value
        self.logger.info(f"Checking {can_id:X}")
        self.voltage_state = state_interfaces["battery/voltage"]
        self.current_state = state_interfaces["battery/current"]
        self.bus.add_callback(can_id, self.get_battery_frame)

    def on_read(self, now, period):
        pass

    def get_battery_frame(self, frame):
        """
        Data format and CANID can be found in https://www.notion.so/MNR-CANBUS-Standards-9dc47508ed3e4dfda2aa9ae97fe1ad54, 
        section: CAN 0 (BLCMDs, LED-Strip, Gimbal CAM, Battery Unit)

        Used for bus.add_callback in on_configure, filters out pack's voltage and current from CanID 0x4B2. 
        converts current(centiamps) & voltage(mV) received from CAN into voltage and amps to be used for the BatteryState publisher.
        """
        self.logger.info(f"Received data: {frame.data} from: {frame.id:X}")
        current = int.from_bytes(frame.data[0:2], 'big', signed=True) 
        voltage = int.from_bytes(frame.data[2:4], 'big', signed=False) 

        self.parsed_current = current/100.0
        self.parsed_voltage = voltage/1000.0
        self.voltage_state.value = self.parsed_voltage
        self.current_state.value = self.parsed_current
        self.logger.info(f"current (A): {self.parsed_current}, voltage (V): {self.parsed_voltage}")

    def on_write(self, now, period):
        pass


if __name__ == '__main__':
    print("Setting up!")
    rclpy.init()

    node = Node("battery_state")

    PythonControl(node, update_rate=5, can_bus="can0") \
        .with_hardware("battery_hw", BatteryStateHardware) \
        .with_controller("battery_controller", BatteryStateController) \
        .with_jcan() \
        .spin()