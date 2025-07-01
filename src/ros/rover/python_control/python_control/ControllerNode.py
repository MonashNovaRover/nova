#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Node Abstract Class
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ControllerNode
TOPICS:
  - subscriber: /control/input_joystick_l [InputJoystick]
  - subscriber: /control/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    python_control
AUTHOR(S):	Tristan Clark
CREATION:	03/05/2024
EDITED:		03/05/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import abc
from python_control.controllers.Controller import Controller
from python_control.limits.Limit import Limit
from python_control.sensors.Sensor import Sensor
import jcan, logging
from rclpy.node import Node

# import the joystick ROS message we are listening to


class ControllerNode(Node, metaclass=abc.ABCMeta):
    # ROS parameter names
    CAN_BUS_PARAM = "can_bus"
    LOGGING_LEVEL_PARAM = "logging_level"
    COMMAND_PERIOD_PARAM = "command_period"

    def __init__(self,  name: str, can_bus: str, log_level: str = "INFO", command_period: float = 0.1):
        super().__init__(name)

        self.declare_parameter(self.CAN_BUS_PARAM, can_bus)
        self.declare_parameter(self.LOGGING_LEVEL_PARAM, log_level)
        self.declare_parameter(self.COMMAND_PERIOD_PARAM, command_period)

        self.get_logger().set_level(logging.getLevelNamesMapping()[self.get_parameter(self.LOGGING_LEVEL_PARAM).value])
        self.get_logger().info(f"{self.get_name()} starting")

        self.bus = jcan.Bus()

        self.controllers : dict[str, Controller] = {}
        self.limits : dict[str, Limit] = {}
        self.sensors : dict[str, Sensor] = {}

        self.timer_send_commands = self.create_timer(self.get_parameter(self.COMMAND_PERIOD_PARAM).value, self.callback_send_commands)
        self.timer_jcan_spin = self.create_timer(0.01, self.bus.spin)

        self.get_logger().info(f"{self.get_name()} started")

    def start_can(self):
        """Setup the CAN bus"""
        self.bus.open(self.get_can_bus())

    def get_can_bus(self):
        return self.get_parameter(self.CAN_BUS_PARAM).value

    def add_controller(self, controller_name: str, controller: Controller):
        self.controllers[controller_name] = controller

    def add_sensor(self, sensor_name: str, sensor: Sensor):
        self.sensors[sensor_name] = sensor

    def callback_send_commands(self):
        """Take current internal state and publish over CAN
        Sends can commands to the controllers
        """
        try:
            for controller in self.controllers.values():
                controller.control_send_callback()

        except Exception as e:
            print(e)

    def deadline_callback(self, info):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.stop_state()

    def stop_state(self):
        """
        Stop all motors
        """
        for controller in self.controllers.values():
            controller.stop()

