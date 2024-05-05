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
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration

# import the joystick ROS message we are listening to
from input_interfaces.msg import InputJoystick


class ControllerNode(abc.ABC, Node):
    # ROS parameter names
    CAN_BUS_PARAM = "can_bus"
    LOGGING_LEVEL_PARAM = "logging_level"

    def __init__(self,  name: str, can_bus: str, log_level: str = "INFO"):
        super().__init__(name)

        self.declare_parameter(self.CAN_BUS_PARAM, can_bus)
        self.declare_parameter(self.LOGGING_LEVEL_PARAM, log_level)

        self.set_logging_level(self.get_parameter(self.LOGGING_LEVEL_PARAM).value)
        self.get_logger().info(f"{name} starting")

        self.bus = jcan.Bus()

        self.joystick_lock = True

        self.controllers : dict[str, Controller] = {}
        self.limits : dict[str, Limit] = {}
        self.sensors : dict[str, Sensor] = {}

        deadline = Duration(nanoseconds=2e8)
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        self.joystick_l_sub = self.create_subscription(InputJoystick, "/inputs/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)
        self.joystick_r_sub = self.create_subscription(InputJoystick, "/inputs/input_joystick_r", self.joystick_r_callback, self.qos, event_callbacks=events)

        self.timer_send_commands = self.create_timer(0.05, self.callback_send_commands)
        self.timer_jcan_spin = self.create_timer(0.01, self.bus.spin)

        self.get_logger().info(f"{self.get_name()} started")
        self.get_logger().info("Joysticks Locked")

    def set_logging_level(self, level: str):
        if level == "DEBUG":
            self.get_logger().set_level(logging.DEBUG)
        elif level == "INFO":
            self.get_logger().set_level(logging.INFO)
        elif level == "WARNING" or level == "WARN":
            self.get_logger().set_level(logging.WARNING)
        elif level == "ERROR":
            self.get_logger().set_level(logging.ERROR)
        elif level == "CRITICAL":
            self.get_logger().set_level(logging.CRITICAL)
        else:
            self.get_logger().set_level(logging.INFO)
            self.get_logger().warning(f"Invalid log level {level}, setting to INFO")

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

    def check_joystick_lock(self):
        if self.joystick_lock:
            self.stop_state()
            return True
        return False

    def update_joystick_lock(self, joystick_l: InputJoystick):
        # Joysticks lock if botton L2 button is pressed on the left joystick
        if joystick_l.btn_bottom_l2_state >= 1 and not self.joystick_lock:
            self.get_logger().info("Joysticks Locked")
            self.joystick_lock = True

        # Joysticks unlocked when bottom l5 button is pressed on the left joystick
        if joystick_l.btn_bottom_l5_state >= 1 and self.joystick_lock:
            self.get_logger().info("Joysticks Unlocked")
            self.joystick_lock = False    

    def joystick_l_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.get_logger().debug("Left Joystick")

        self.update_joystick_lock(msg)

        if self.check_joystick_lock():
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
        
        self.joystick_r(msg)
        
    @abc.abstractmethod
    def joystick_r(self, joystick_r: InputJoystick):
        pass

    @abc.abstractmethod
    def joystick_l(self, joystick_l: InputJoystick):
        pass
  
        







