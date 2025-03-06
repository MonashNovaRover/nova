#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control for the Spinny Part of the ARC
analysis arm
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: SweepyServoNode
TOPICS:
  - subscriber: /inputs/input_joystick_l [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):	Brandon Chung
CREATION:	05/03/2025
EDITED:		06/03/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from python_control.ControllerNode import ControllerNode
from python_control.controls.Direction import Direction
from input_interfaces.msg import InputJoystick
from python_control.controls.ContinuousOneAxisPositionControl import ContinuousOneAxisPositionControl
from python_control.controllers.JonoPositionController import JonoPositionController


class SweepyNode(ControllerNode):
    # CAN BUS NAME
    CAN_BUS = "can1"

    # SENDING CARD IDS
    SERVO_ID = 0x0F1 # todo adjust accordingly

    # SENDING COMMAND IDS
    MOVE_SERVO_COMMAND = 0x10

    # BASE VELOCITY
    DEFAULT_BASE_VELOCITY = 5
    BASE_VELOCITY_PARAM = "base_velocity_param"

    def __init__(self):
        super().__init__(name="SweepyServoNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.velocity_multiplier = 1

        self.declare_parameter(self.BASE_VELOCITY_PARAM, self.DEFAULT_BASE_VELOCITY)

        ## Create CONTROLS
        self.sweepy_servo = ContinuousOneAxisPositionControl(
            logger=logger,
        )

        ## Create CONTROLLERS
        self.sweepy_servo_controller = JonoPositionController(
            logger=logger,
            bus=self.bus,
            pos_command=self.MOVE_SERVO_COMMAND,
            frame_id=self.SERVO_ID,
            control=self.spinny_part,
        )

        ## Add the CONTROLLERS to the node's controllers
        self.add_controller(self.sweepy_servo, self.sweepy_servo_controller)

        ## Start the CAN bus
        self.start_can()

    def joystick_l(self, joystick_l: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.velocity_multiplier = abs(joystick_l.ax_slider)

        match joystick_l.btn_thumb_u_state:
            case 1, 2: # Spin the sweeper
                self.get_logger().debug("Spinning sweepy")
                self.spinny_part.displace(
                self.velocity_multiplier
                * self.get_parameter(self.BASE_VELOCITY_PARAM).value
                )

def main():
    rclpy.init()
    node = SweepyNode()
    rclpy.spin(node)
    rclpy.shutdown()
