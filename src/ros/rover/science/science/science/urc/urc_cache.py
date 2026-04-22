#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controls the URC Caches
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: CacheController
COMMAND INTERFACES:
  - actuation/effort    [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Yahya Muayyiduddin
CREATION:       17/2/2026
EDITED:         21/2/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
import jcan
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface

from python_control2.hardware_interfaces import CMDHardware, ContinousServoHardware
from teleop_python_utils import Inputs


class CacheController(Controller):
    # Command interfaces
    # joint_cmd: Interface

    # State interfaces
    # state: Interface

    def __init__(self, contexts: Contexts, cache_name: str):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"CacheController -- I have been __init__ialized")

        # Do any setup logic here, save any contexts you want reference to in the future.
        # Save Input references here
        # self.button_name = self.declare_parameter("button", button).value
        self.cache_name = self.declare_parameter("cache_name", cache_name).value
        self.cache_axis_name = self.declare_parameter("cache_axis", f"{self.cache_name}_actuation").value
        self.cache_speed_axis_name = self.declare_parameter("cache_speed", f"{self.cache_name}_speed").value


        inputs = contexts[Inputs]

        self.cache_axis = inputs.get_axis(self.cache_axis_name)

        self.cache_speed = inputs.get_axis(self.cache_speed_axis_name)


    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Save references to interfaces
        self.cache_cmd = command_interfaces["actuation/effort"]


    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        # Update Command Interfaces
        self.cache_cmd.value = self.cache_axis.value * self.get_speed()


    
    def get_speed(self) -> float:
        """ gets the speed, turning an axis [-1, 1] to a speed [0, 1]"""
        return (self.cache_speed.value + 1) / 2  
        

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("cache")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", CacheController, cache_name = "cache_left") \
        .with_hardware("actuation", ContinousServoHardware, can_id=0x0E2) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()