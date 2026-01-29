#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller that takes axis inputs and outputs
corresponding effort to an effort hardware.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <hardware_name>/effort    [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Felicity Matthews
CREATION:       28/01/26
EDITED:         28/01/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from typing import Optional
from ..controller_manager.Interface import Interface, InterfaceCollection
from ..controller_manager.Contexts import Contexts
from ..controller_manager.Activation import Activation
from .Controller import Controller
from teleop_python_utils import Inputs


class ActuateController(Controller):
    # Command interfaces
    actuation_cmd: Interface

    def __init__(self, contexts: Contexts, hardware_name: str="hardware_name", actuation_axis: str="actuation"):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"ActuateController -- I have been __init__ialized")

        self.active = contexts[Activation]

        # Get inputs
        self.hardware_name = self.declare_parameter("hardware_name", hardware_name).value

        # Actuation axis
        self.actuation_axis_name = self.declare_parameter("actuation_axis", actuation_axis).value

        inputs = contexts[Inputs]
        self.actuation_axis = inputs.get_axis(self.actuation_axis_name)

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
        self.logger.info(f"Getting {self.hardware_name}/effort")
        self.actuation_cmd = command_interfaces[f"{self.hardware_name}/effort"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        if not self.active:
            self.actuation_cmd.value = 0
            return

        # Update actuation
        self.actuation_cmd.value = self.actuation_axis.value
