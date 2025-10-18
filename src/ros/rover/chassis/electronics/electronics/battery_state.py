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
CREATION:       
EDITED:         October 18 2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
to run with parameter file:
$ ros2 run electronics battery_state.py --ros-args --params-file ~/nova/src/ros/rover/nova_bringup/params/battery_state.yaml
in testing ws (del later):
$ ~/Builds/testing/bin/ros2 run electronics battery_state.py --ros-args --params-file ~/nova/src/ros/rover/nova_bringup/params/battery_state.yaml

TODO 
testing instructions:
for writing ros2 node be like run this command this is the expected output and can command and epeted behaviour and publish 

parametarise the voltage of canID self.declare parameter in init and set the id as the default id
prefix state with battery/voltage and battery/current
also ask electrical if theyd rather voltage and amps or keep as mv and centiamps
meow meow meow meow
"""

import rclpy
from rclpy.node import Node, ParameterDescriptor
# from python_control.ControllerNode import ControllerNode
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
        self.voltage_state = state_interfaces["voltage"]
        self.current_state = state_interfaces["current"]
        self.logger.info("Getting state of current and voltage")
        return True

    def on_update(self, now, period):
        """ 
        Purpose: Publishes most recent battery voltage and current to /battery_state of type sensor_msg.msg/BatteryState
        msg type can be found here!
        https://docs.ros.org/en/jade/api/sensor_msgs/html/msg/BatteryState.html
        """
        #BatteryState here is the ROS2 publisher node
        msg = BatteryState()

        if math.isnan(self.voltage_state.value):
            msg.present = False 
            msg.voltage = float('nan')
            msg.current = float('nan')

        msg.voltage = self.voltage_state.value
        msg.present = True
        msg.current = self.current_state.value

        self.publisher_battery_state.publish(msg)

class BatteryStateHardware(HardwareInterface):

    VOLTAGE_CURRENT_CANID = 0x4B2

    def __init__(self, contexts: Contexts):
        super().__init__(contexts)
        self.logger.info(f"Hardware interface initialised")
        self.bus = contexts[jcan.Bus]

        self.voltage = float('nan')
        self.current = float('nan')

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        self.logger.info(f"Checking {self.VOLTAGE_CURRENT_CANID:X}")
        self.voltage_state = state_interfaces["voltage"]
        self.current_state = state_interfaces["current"]
        self.bus.add_callback(self.VOLTAGE_CURRENT_CANID, self.get_battery_frame)

    def on_read(self, now, period):
        pass

    def get_battery_frame(self, frame):
        """
        Important:
        Data format and CANID can be found in https://www.notion.so/MNR-CANBUS-Standards-9dc47508ed3e4dfda2aa9ae97fe1ad54, 
        section: CAN 0 (BLCMDs, LED-Strip, Gimbal CAM, Battery Unit)

        Purpose: Used for bus.add_callback in on_configure, filters out pack's voltage and current from CanID 0x4B2

        How it works: 
        first 2 bytes of data is a 16 bit signed integer in centi-amps where negative current represents current flowing from the pack to the rover
        last 2 bytes of data is a 16 bit unsigned integer representing the pack voltage in mV
        converts current(centiamps) & voltage(mV) received from CAN into voltage and amps and to be used for the BatteryState publisher.
        """
        self.logger.info(f"Received data: {frame.data} from: {frame.id:X}")
        current = int.from_bytes(frame.data[0:2], 'big', signed=True) 
        voltage = int.from_bytes(frame.data[2:4], 'big', signed=False) 

        self.parsed_current = current/100.0
        self.parsed_voltage = voltage/1000.0
        self.voltage_state.value = self.parsed_voltage
        self.current_state.value = self.parsed_current
        self.logger.info(f"voltage (V): {self.parsed_voltage}. current (A): {self.parsed_current}")

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