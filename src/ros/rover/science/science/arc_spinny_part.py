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
from python_control.JoystickControllerNode import JoystickControllerNode
from python_control.controls.Direction import Direction
from input_interfaces.msg import InputJoystick
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controllers.JonoVelocityController import JonoVelocityController


class SpinnyPartNode(JoystickControllerNode):
    # CAN BUS NAME
    CAN_BUS = "can1"

    # SENDING CARD IDS
    SERVO_ID = 0x0F1 # todo adjust accordingly

    # SENDING COMMAND IDS
    SPIN_CLOCKWISE = 0x03 # todo adjust
    SPIN_ANTICLOCKWISE = 0x04 # todo adjust

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    SPIN_MAX_PERCENT = 0.7
    SPIN_CONTROL_NAME = "Analysis Arm Spinny Part"

    # CONTROL DIRECTIONS
    DIRECTION_CLOCKWISE = Direction.POSITIVE
    DIRECTION_ANTICLOCKWISE = Direction.NEGATIVE

    def __init__(self):
        super().__init__(name="SpinnyPartNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.velocity = 0.5

        ## Create CONTROLS
        self.spinny_part = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.SPIN_MAX_PERCENT,
        )

        ## Create CONTROLLERS
        self.spinny_part_controller = JonoVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.SERVO_ID,
            pos_command=self.SPIN_CLOCKWISE,
            neg_command=self.SPIN_ANTICLOCKWISE,
            control=self.spinny_part,
        )

        ## Add the CONTROLLERS to the node's controllers
        self.add_controller(self.SPIN_CONTROL_NAME, self.spinny_part_controller)

        ## Start the CAN bus
        self.start_can()

    def joystick_l(self, joystick_l: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.velocity = abs(joystick_l.ax_slider)
        self.get_logger().debug(f"Velocity updated to {self.velocity}")

        if joystick_l.btn_thumb_l_state >= 1:
            self.get_logger().info(f"Spinny part moving ANTICLOCKWISE at velocity {self.velocity}")
            self.spinny_part.update_direction(self.DIRECTION_ANTICLOCKWISE)
            self.spinny_part.update_velocity(self.velocity)
        elif joystick_l.btn_thumb_r_state >= 1:
            self.get_logger().info(f"Spinny part moving CLOCKWISE at velocity {self.velocity}")
            self.spinny_part.update_direction(self.DIRECTION_CLOCKWISE)
            self.spinny_part.update_velocity(self.velocity)
        else:
            self.spinny_part.stop()

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

if __name__=="__main__":
    main()
