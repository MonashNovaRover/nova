#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the kiln door. It opens and closes and current
senses the door motor.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: KilnDoorController

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Angel Aguinaldo
CREATION:       20/02/26
EDITED:         20/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
import jcan
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface, Direction

from python_control2.hardware_interfaces import QCMDHardware, GenericSensorHardware
from teleop_python_utils import Inputs


class KilnDoorController(Controller):
    # Command interfaces
    kiln_door_cmd: Interface
    # State interfaces
    door_current_state: Interface

    def __init__(self, contexts: Contexts, max_door_current:int):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"KilnDoor -- I have been __init__ialized")

        #ros2 parameters
        self.door_actuation_axis_name = self.declare_parameter("actuation_axis", "door_actuation").value
        self.door_speed_name = self.declare_parameter("speed_axis", "door_speed").value

        #maximum door current 
        self.max_door_current = self.declare_parameter("max_door_current", max_door_current)

        #inputs
        inputs = contexts[Inputs]

        self.door_actuation = inputs.get_axis(self.door_actuation_axis_name)
        self.door_speed = inputs.get_axis(self.door_speed_name)


        self.door_state = Direction.POSITIVE #1:open -1:closed

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
        self.kiln_door_cmd = command_interfaces["kiln_door/effort"]

        #door current state interface
        self.door_current_state = state_interfaces["current_sensor/current"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        # Update Command Interfaces

        #update door state
        if self.door_state!= self.door_actuation.value:
            self.logger.info(f"Kiln door {"CLOSING" if self.door_state == Direction.POSITIVE else ("OPENING" if self.door_state == Direction.NEGATIVE else pass)}")
        
        self.door_state = self.door_actuation.value

        #check if door current reached max val
        if abs(self.door_current_state.value) < self.max_door_current.value:
            #set command interface
            self.kiln_door_cmd.value = self.door_state * self.axis_to_speed(self.door_speed.value)
        else:
            #turn off motor
            self.logger.info("Current limit reached! Turning off motor...")
            self.kiln_door_cmd.value = 0

    def axis_to_speed(self, axis:int):
        return (axis +1)/2

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("kiln_door")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", KilnDoorController, max_door_current = 0x3000) \
        .with_hardware("kiln_door", QCMDHardware, can_id = 0xD2) \
        .with_hardware("current_sensor",GenericSensorHardware, can_id=0x4FF, unit = "current") \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()