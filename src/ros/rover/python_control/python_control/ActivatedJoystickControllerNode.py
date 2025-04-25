#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Activated Joystick Control Node Abstract Class
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ActivatedJoystickControllerNode
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    python_control
AUTHOR(S):	Felicity Matthews
CREATION:	21/04/2025
EDITED:		25/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import abc
from rcl_interfaces.msg import ParameterDescriptor
from python_control.JoystickControllerNode import JoystickControllerNode

# import the joystick ROS message we are listening to
from input_interfaces.msg import InputJoystick

class ActivatedJoystickControllerNode(JoystickControllerNode, metaclass=abc.ABCMeta):
    """ A Joystick controller node that activates and deactivated the node depending on what Joystick buttons are pressed.
    """
    # ROS parameter names

    def __init__(self, name: str, can_bus: str, log_level: str = "INFO", command_period: float = 0.1):
        super().__init__(name=name, can_bus=can_bus, log_level=log_level, command_period=command_period)

        self.active: bool = self.declare_parameter("active", True, ParameterDescriptor(description='On start node status')).value
        self.active_button: str = self.declare_parameter("active_button", "", ParameterDescriptor(description='InputJoystick button name that activates the node')).value
        self.inactive_button_pool: list[str] = self.declare_parameter("inactive_button_pool", [""], ParameterDescriptor(description='list of InputJoystick button name that deactivates the node')).get_parameter_value().string_array_value
        self.using_left_joystick: bool = self.declare_parameter("using_left_joystick", True, ParameterDescriptor(description='Whether or not the buttons are on the left joystick')).value

        self.get_logger().info(f"{self.get_name()} is active: {self.active}")

    def check_active_status(self):
        """
        Checks the current status of the node and returns the status
        :return: bool
        """
        if not self.active:
            self.stop_state()
            return False
        return True

    def update_active_status(self, msg: InputJoystick):
        """
        Updates the active status from the Joystick input
        :return: None
        """
        if not self.active_button:
            return

        # check active button
        if not self.active and getattr(msg, self.active_button) > 0:
            self.active = True
            self.get_logger().info(f"{self.get_name()} ACTIVATED")

        if not self.active:
            return

        # check inactive buttons
        for button in self.inactive_button_pool:
            if getattr(msg, button) > 0:
                self.active = False
                self.get_logger().info(f"{self.get_name()} DEACTIVATED")
                break

    def joystick_l_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.get_logger().debug("Left Joystick")

        self.update_joystick_lock(msg)

        if self.check_joystick_lock():
            return

        if self.active_button:
            if self.using_left_joystick:
                self.update_active_status(msg)

            if not self.check_active_status():
                return

        self.joystick_l(msg)

    def joystick_r_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.get_logger().debug("Right Joystick")

        if self.check_joystick_lock():
            return

        if self.active_button:
            if not self.using_left_joystick:
                self.update_active_status(msg)

            if not self.check_active_status():
                return

        self.joystick_r(msg)

    @abc.abstractmethod
    def joystick_r(self, joystick_r: InputJoystick):
        pass

    @abc.abstractmethod
    def joystick_l(self, joystick_l: InputJoystick):
        pass
