#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the science sweeper which spins.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: Sweeper
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - sweep/effort    [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Brandon Chung
CREATION:       05/02/26
EDITED:         05/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, Direction, Activation
from python_control2.hardware_interfaces import QCMDHardware
from teleop_python_utils import Inputs


class SweeperController(Controller):
    # Command interfaces
    sweep_cmd: Interface

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"SweeperController -- I have been __init__ialized")

        self.sweeper_direction = Direction.POSITIVE
        self.active = contexts[Activation]

        # Get inputs
        # Sweeper axis
        self.sweeper_axis_name = self.declare_parameter("sweeper_axis", "sweeper_sweep").value

        inputs = contexts[Inputs]
        self.actuation_axis = inputs.get_axis(self.actuation_axis_name)


        # Activate sweeper
        self.sweeper_axis = inputs.get_axis(self.sweeper_axis)

        inputs.get_axis(self.sweeper_axis_name).add_callback(self.update_sweeper_sweep())

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
        self.logger.info(f"Getting sweep/effort")
        self.sweep_cmd = command_interfaces["sweep/effort"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        if not self.active:
            self.sweep_cmd.value = self.to_16bit(0)
            return

        # Update Command Interfaces
        # Update sweeping
        # Convert to 16 bit integer
        self.sweep_cmd.value = self.to_int16(self.get_sweep_speed() * self.sweep_direction.value)

    def update_sweep_direction(self):
        def update_sweep():
            # Check if sweeping direction has changed
            match (new_direction := self.get_sweeper_direction()):
                case self.sweeper_direction:
                    return
                case Direction.POSITIVE:
                    self.sweeper_direction = Direction.POSITIVE
                case Direction.NEGATIVE:
                    self.sweeper_direction = Direction.NEGATIVE
                case _:
                    self.logger.error(f"Updated to invalid sweeper direction: {new_direction}")

            self.logger.info(f"Updated sweeper direction: {"CLOCKWISE" if self.sweeper_direction == Direction.POSITIVE else "ANTICLOCKWISE"}")
        return update_sweep

    def get_sweeper_speed(self) -> float:
        """ gets the sweeper speed, turning an axis [-1, 1] to a speed [0, 1]"""
        return abs(self.sweeper_axis.value)

    def get_sweeper_direction(self) -> float:
        """ gets the sweeper direction, turning an axis [-1, 1] to -1 or 1"""
        if self.sweeper_axis.value >= 0:
            return Direction.POSITIVE
        else:
            return Direction.NEGATIVE

    def to_int16(self, number: float) -> bytes:
        return int(number).to_bytes(2, byteorder='big', signed=True)


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("sweeper")
    inputs = Inputs(node).with_topics("/science/input")

    # ARCh auger system
    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("controller", SweeperController) \
        .with_hardware("sweeper", QCMDHardware, can_id=0x0E4) \
        .with_teleop(inputs) \
        .with_activation_buttons(start_active=True, active_button_name="activate_sweeper", inactive_button_pool_names=["activate_cbeam","activate_auger"]) \
        .with_jcan() \
        .spin()
