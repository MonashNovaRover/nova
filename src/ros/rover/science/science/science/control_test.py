#!/usr/bin/env python3

from typing import Optional

from python_control2.controller_manager.ControllerManagerBuilder import ControllerManagerBuilder
from python_control2.controller_manager.Interface import InterfaceCollection
from python_control2.controller_manager.Contexts import Contexts
from python_control2.controllers.Controller import Controller


class TestController(Controller):
    def __init__(self, contexts: Contexts):
        self.logger.info("I have been __init__ialized")

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param command_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        self.logger.info("Configuring!")
        return True

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        self.logger.info("Update!")
        pass


if __name__ == "__main__":
    print("Setting up!")

    ControllerManagerBuilder.NewControllerManager("control_test")\
        .with_controller("test_controller", TestController())\
        .spin()