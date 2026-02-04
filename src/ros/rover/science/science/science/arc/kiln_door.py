#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
<insert purpose here>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: KilnDoor
TOPICS:
  - publisher: <topic> [<msg type>]
SERVICES:
	- service: <service> [<srv type>]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        kiln_door
AUTHOR(S):      <insert your name>
CREATION:       <current date>
EDITED:         <current date>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
import jcan
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface, Direction

from python_control2.hardware_interfaces import CMDHardware
from teleop_python_utils import Inputs


class KilnDoorController(Controller):
    # Command interfaces
    # joint_cmd: Interface
    kiln_door_cmd: Interface
    door_current_state: Interface
    # State interfaces

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"KilnDoor -- I have been __init__ialized")

        #ros2 parameters
        self.door_open_btn_name = self.declare_parameter("open_button", "open_kiln_door").value
        self.door_close_btn_name = self.declare_parameter("close_button", "close_kiln_door").value
        self.door_speed_name = self.declare_parameter("speed_axis", "door_speed").value

        #maximum door current 
        self.max_door_current = self.declare_parameter("max_door_current")

        #inputs
        inputs = contexts[Inputs]

        self.door_open_btn  = inputs.get_button(self.door_open_btn_name)
        self.door_close_btn = inputs.get_button(self.door_close_btn_name)
        self.door_speed = inputs.get_axis(self.door_speed_name)


        self.door_state = Direction.POSITIVE #1:open -1:closed

        #button callback functions
        self.door_open_btn.add_callback(self.update_door_direction())
        self.door_close_btn.add_callback(self.update_door_direction())


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
        # self.logger.info(f"Getting \"{self.joint + "/effort"}\"")
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
        # self.logger.info(f"{self.state.value} -> {self.cmd.value}")

        #check if door current reached max val
        if abs(self.door_current_state) < self.max_door_current.value:
            #set command interface
            self.kiln_door_cmd.value = self.door_state * self.axis_to_speed(self.door_speed.value)
        else:
            #turn off motor
            self.kiln_door_cmd.value = 0
    
    def update_door_direction(self, direction:Direction):
        """
            Changes whether the kiln door is opening or closing during a button press.
        """
        def change_direction():
            if self.door_state != direction:
                self.logger.info(f"Kiln door {"CLOSING" if self.state == Direction.POSITIVE else "OPENING"}")
                self.door_state = direction
        return change_direction

    def axis_to_speed(axis:int):
        return (axis +1)/2

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("kiln_door")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl("kiln_door", update_rate=5, can_bus="can1") \
        .with_controller("controller", KilnDoorController) \
        .with_hardware("kiln_door", QCMDHardware, can_id = "") \
        .with_hardware("current_sensor",GenericSensorHardware, can_id="", unit = "amp")
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()