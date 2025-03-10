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
from python_control.controls.Direction import Direction
from input_interfaces.msg import InputJoystick
from python_control.JoystickControllerNode import JoystickControllerNode
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controllers.JonoVelocityController import JonoVelocityController


class SweeperNode(JoystickControllerNode):
    # CAN BUS NAME
    CAN_BUS = "can1"

    # SENDING CARD IDS
    SERVO_ID = 0x0F1 # todo adjust accordingly

    # SENDING COMMAND IDS
    SWEEP_CLOCKWISE = 0x01 # todo adjust
    SWEEP_ANTICLOCKWISE = 0x02 # todo adjust

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    SWEEPER_MAX_PERCENT = 0.7

    # CONTROL DIRECTIONS
    DIRECTION_CLOCKWISE = Direction.POSITIVE
    DIRECTION_ANTICLOCKWISE = Direction.NEGATIVE


    def __init__(self):
        super().__init__(name="SweepyServoNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.velocity = 0.5

        ## Create CONTROLS
        self.sweeper_servo = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.SWEEPER_MAX_PERCENT,
        )

        ## Create CONTROLLERS
        self.sweeper_servo_controller = JonoVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.SERVO_ID,
            pos_command=self.SWEEP_CLOCKWISE,
            neg_command=self.SWEEP_ANTICLOCKWISE,
            control=self.sweeper_servo,
        )

        ## Add the CONTROLLERS to the node's controllers
        self.add_controller(self.sweeper_servo, self.sweeper_servo_controller)

        ## Start the CAN bus
        self.start_can()

    def joystick_r(self, joystick_r: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        if joystick_r.btn_thumb_l_state >= 1:
            self.get_logger().info("Sweeper moving ANTICLOCKWISE")
            self.sweeper_servo.update_direction(self.DIRECTION_ANTICLOCKWISE)
            self.sweeper_servo.update_velocity(self.velocity)
        elif joystick_r.btn_thumb_r_state >= 1:
            self.get_logger().info("Sweeper moving CLOCKWISE")
            self.sweeper_servo.update_direction(self.DIRECTION_CLOCKWISE)
            self.sweeper_servo.update_velocity(self.velocity)
        else:# Called when the script executes
            self.sweeper_servo.stop()

    def joystick_l(self, joystick_l: InputJoystick):
        """
        Right joystick callback function
        """
        self.velocity = abs(joystick_l.ax_slider)
        self.get_logger().debug(f"Velocity updated to {self.velocity}")

def main():
    rclpy.init()
    node = SweeperNode()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__=="__main__":
    main()
