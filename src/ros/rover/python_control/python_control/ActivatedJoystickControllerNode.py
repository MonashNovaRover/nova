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
EDITED:		21/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import abc
from python_control.JoystickControllerNode import JoystickControllerNode

# import the joystick ROS message we are listening to
from input_interfaces.msg import InputJoystick


class ActivatedJoystickControllerNode(JoystickControllerNode, metaclass=abc.ABCMeta):
    # ROS parameter names

    def __init__(self, name: str, can_bus: str, log_level: str = "INFO", command_period: float = 0.1):
        super().__init__(name=name, can_bus=can_bus, log_level=log_level, command_period=command_period)

        self.active = self.declare_parameter("active", True).value
        self.active_button = self.declare_parameter("active_button", "").value
        self.button_pool = self.declare_parameter("button_pool", []).value
        self.using_left_joystick = self.declare_parameter("using_left_joystick", True).value

        self.get_logger().info(f"{name} is active: {self.active}")

    def check_active_status(self):
        if not self.active:
            self.stop_state()
            return False
        return True

    def update_active_status(self, msg: InputJoystick):
        if not self.active_button:
            return

        # logic here for checking if active

        pass

    def joystick_l_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        if self.active_button:
            if self.using_left_joystick:
                self.update_active_status(msg)

            if not self.check_active_status():
                return

        super().joystick_l_callback(msg)


    def joystick_r_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        if self.active_button:
            if not self.using_left_joystick:
                self.update_active_status(msg)

            if not self.check_active_status():
                return

        super().joystick_l_callback(msg)

    @abc.abstractmethod
    def joystick_r(self, joystick_r: InputJoystick):
        pass

    @abc.abstractmethod
    def joystick_l(self, joystick_l: InputJoystick):
        pass
