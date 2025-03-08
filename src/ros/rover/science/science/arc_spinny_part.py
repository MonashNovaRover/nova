#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control for the Spinny Part of the ARC
analysis arm
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: SpinnyPartNode
TOPICS:
  - subscriber: /inputs/input_joystick_l [InputJoystick]
  - subscriber: /inputs/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):	Felicity Matthews
CREATION:	02/03/2025
EDITED:		02/03/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from python_control.ControllerNode import ControllerNode
from python_control.controls.Direction import Direction
from input_interfaces.msg import InputJoystick
from python_control.controls.ContinuousOneAxisPositionControl import ContinuousOneAxisPositionControl
from python_control.controllers.JonoPositionController import JonoPositionController


class SpinnyPartNode(JoystickControllerNode):
    # CAN BUS NAME
    CAN_BUS = "can1"

    # SENDING CARD IDS
    SERVO_ID = 0x0F1 # todo adjust accordingly

    # CONTROL PARAMETERS
    SERVO_MAX_POS = 360

    # SENDING COMMAND IDS
    MOVE_SERVO_COMMAND = 0x10

    # CONTROL DIRECTIONS
    POSITION_CLOCKWISE = Direction.POSITIVE
    POSITION_ANTICLOCKWISE = Direction.NEGATIVE

    # BASE VELOCITY
    DEFAULT_BASE_VELOCITY = 5
    BASE_VELOCITY_PARAM = "base_velocity_param"

    def __init__(self):
        super().__init__(name="SpinnyPartNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.velocity_multiplier = 1

        self.declare_parameter(self.BASE_VELOCITY_PARAM, self.DEFAULT_BASE_VELOCITY)

        ## Create CONTROLS
        self.spinny_part = ContinuousOneAxisPositionControl(
            logger=logger,
            max_angle=self.SERVO_MAX_POS,
        )

        ## Create CONTROLLERS
        self.spinny_part_controller = JonoPositionController(
            logger=logger,
            bus=self.bus,
            pos_command=self.MOVE_SERVO_COMMAND,
            frame_id=self.SERVO_ID,
            control=self.spinny_part,
            max_value=self.SERVO_MAX_POS,
        )

        ## Add the CONTROLLERS to the node's controllers
        self.add_controller(self.spinny_part, self.spinny_part_controller)

        ## Start the CAN bus
        self.start_can()

    def joystick_l(self, joystick_l: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.velocity_multiplier = abs(joystick_l.ax_slider)

        if joystick_l.btn_thumb_l_state >= 1:
            self.get_logger().debug("Spinny Part ANTICLOCKWISE")
            self.spinny_part.displace(
                self.velocity_multiplier
                * self.get_parameter(self.BASE_VELOCITY_PARAM).value
                * self.POSITION_ANTICLOCKWISE
            )
        elif joystick_l.btn_thumb_r_state >= 1:
            self.get_logger().debug("Spinny Part CLOCKWISE")
            self.spinny_part.displace(
                self.velocity_multiplier
                * self.get_parameter(self.BASE_VELOCITY_PARAM).value
                * self.POSITION_CLOCKWISE
            )

    def joystick_r(self, joystick_r: InputJoystick):
        """
        Right joystick callback function
        """
        pass

def main():
    rclpy.init()
    node = SpinnyPartNode()
    rclpy.spin(node)
    rclpy.shutdown()
