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
from python_control.ControllerNode import ControllerNode
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration

# import the joystick ROS message we are listening to
from input_interfaces.msg import InputJoystick


class JoystickControllerNode(ControllerNode, metaclass=abc.ABCMeta):
    # ROS parameter names

    def __init__(self,  name: str, can_bus: str, log_level: str = "INFO", command_period: float = 0.1):
        super().__init__(name=name,can_bus=can_bus, log_level=log_level, command_period=command_period)

        self.joystick_lock = True

      
        deadline = Duration(nanoseconds=2e8)
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        self.joystick_l_sub = self.create_subscription(InputJoystick, "/inputs/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)
        self.joystick_r_sub = self.create_subscription(InputJoystick, "/inputs/input_joystick_r", self.joystick_r_callback, self.qos, event_callbacks=events)

        self.get_logger().info("Joysticks Locked")


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
  
        







