#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the science Auger which actuates up
and down and drills.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: AugerController
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       13/01/26
EDITED:         13/01/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, Direction, Activation
from python_control2.hardware_interfaces import QCMDHardware
from teleop_python_utils import Inputs


class AugerController(Controller):
    # Command interfaces
    actuation_cmd: Interface
    drill_cmd: Interface

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"AugerController -- I have been __init__ialized")

        self.drill_direction = Direction.POSITIVE
        self.active = contexts[Activation]

        # Get inputs
        # Actuation axis
        self.actuation_axis_name = self.declare_parameter("actuation_axis", "auger_actuation").value

        inputs = contexts[Inputs]
        self.actuation_axis = inputs.get_axis(self.actuation_axis_name)

        # Drill activate, speed and direction buttons and axes
        self.drill_button_name = self.declare_parameter("drill_button", "auger_drill").value
        self.speed_axis_name = self.declare_parameter("speed_axis", "auger_speed").value
        self.drill_clockwise_button_name = self.declare_parameter("drill_clockwise_button", "auger_drill_clockwise").value
        self.drill_anticlockwise_button_name = self.declare_parameter("drill_anticlockwise_button", "auger_drill_anticlockwise").value

        self.drill_button = inputs.get_button(self.drill_button_name)
        self.speed_axis = inputs.get_axis(self.speed_axis_name)
        inputs.get_button(self.drill_clockwise_button_name).add_callback(self.update_drill_direction(Direction.POSITIVE))
        inputs.get_button(self.drill_anticlockwise_button_name).add_callback(self.update_drill_direction(Direction.NEGATIVE))

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
        if not self.active:
            self.drill_cmd.value = 0
            self.actuation_cmd.value = 0
            return

        # Update Command Interfaces
        # Update drill speed
        self.drill_cmd.value = self.drill_button.value * self.speed_axis.value * self.drill_direction.value

        # Update actuation
        self.actuation_cmd.value = self.actuation_axis.value

    def update_drill_direction(self, direction: Direction):
        def update_drill():
            self.drill_direction = direction
        return update_drill


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("auger")
    inputs = Inputs(node).with_topics("/science/input")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", AugerController) \
        .with_hardware("actuation", QCMDHardware, can_id=0xC2, max_effort=1) \
        .with_hardware("drill", QCMDHardware, can_id=0xC1, max_effort=1) \
        .with_teleop(inputs) \
        .with_activation_buttons(start_active=True, active_button_name="activate_auger", inactive_button_pool_names=["activate_cbeam"]) \
        .with_jcan() \
        .spin()