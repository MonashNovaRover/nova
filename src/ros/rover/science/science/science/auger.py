#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
<insert purpose here>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: AugerController
TOPICS:
  - publisher: <topic> [<msg type>]
SERVICES:
	- service: <service> [<srv type>]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      <insert your name>
CREATION:       <current date>
EDITED:         <current date>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface

from python_control2.hardware_interfaces import CMDHardware
from teleop_python_utils import Inputs


class AugerController(Controller):
    # Command interfaces
    actuation_cmd: Interface
    drill_cmd: Interface

    # State interfaces
    # state: Interface

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"AugerController -- I have been __init__ialized")
        self.logger.get_child(self.name).info()

        # Do any setup logic here, save any contexts you want reference to in the future.
        # Save Input references here
        self.active_button_name = self.declare_parameter("active_button", f"{self.name}_active").value
        self.inactive_button_pool_names = self.declare_parameter("inactive_button_pool", []).value
        self.active = self.declare_parameter("start_active", True)

        # Get inputs
        self.actuation_axis_name = self.declare_parameter("actuation_axis", "auger_actuation").value
        self.drill_axis_name = self.declare_parameter("drill_axis", "auger_drill").value
        self.speed_axis_name = self.declare_parameter("speed_axis", "auger_speed").value

        inputs = contexts[Inputs]
        self.actuation_input = inputs.get_button(self.actuation_axis_name)
        self.drill_input = inputs.get_axis(self.drill_axis_name)
        self.speed_input = inputs.get_axis(self.speed_axis_name)


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
        self.logger.info(f"Getting actuation/effort and drill/effort")
        self.actuation_cmd = command_interfaces["actuation/effort"]
        self.drill_cmd = command_interfaces["drill/effort"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        # Update Command Interfaces
        # self.cmd.value = 2 * self.state.value
        # self.logger.info(f"{self.state.value} -> {self.cmd.value}")

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("auger")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", AugerController) \
        .with_hardware("actuation", CMDHardware, can_id=0x0C2, max_effort=0.75) \
        .with_hardware("drill", CMDHardware, can_id=0x0C1, max_effort=0.6) \
        .with_teleop(inputs) \
        .with_jcan() \
        .spin()