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
    SERVO_ID_PARAM = "servo_id"
    SERVO_ID = 0x0A0
    COMMAND_PERIOD = 0.3

    # SENDING COMMAND IDS
    SWEEP_CLOCKWISE_PARAM = "clockwise_command"
    SWEEP_CLOCKWISE = 0x01
    SWEEP_ANTICLOCKWISE_PARAM = "anticlockwise_command"
    SWEEP_ANTICLOCKWISE = 0x02

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    SWEEPER_MAX_PERCENT = 1
    SWEEPER_CONTROL_NAME = "Sweeper"

    # CONTROL DIRECTIONS
    DIRECTION_CLOCKWISE = Direction.POSITIVE
    DIRECTION_ANTICLOCKWISE = Direction.NEGATIVE


    def __init__(self):
        super().__init__(name="SweepyServoNode", can_bus=self.CAN_BUS, command_period=self.COMMAND_PERIOD)
        logger = self.get_logger()

        self.velocity = 0.5

        # Declare parameters
        self.declare_parameter(self.SERVO_ID_PARAM, self.SERVO_ID)
        self.declare_parameter(self.SWEEP_CLOCKWISE_PARAM, self.SWEEP_CLOCKWISE)
        self.declare_parameter(self.SWEEP_ANTICLOCKWISE_PARAM, self.SWEEP_ANTICLOCKWISE)

        ## Create CONTROLS
        self.sweeper_servo = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.SWEEPER_MAX_PERCENT,
        )

        ## Create CONTROLLERS
        self.sweeper_servo_controller = JonoVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.get_parameter(self.SERVO_ID_PARAM).value,
            pos_command=self.get_parameter(self.SWEEP_CLOCKWISE_PARAM).value,
            neg_command=self.get_parameter(self.SWEEP_ANTICLOCKWISE_PARAM).value,
            control=self.sweeper_servo,
        )

        ## Add the CONTROLLERS to the node's controllers
        self.add_controller(self.SWEEPER_CONTROL_NAME, self.sweeper_servo_controller)

        ## Start the CAN bus
        self.start_can()

    def joystick_r(self, joystick_r: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.velocity = abs(joystick_r.ax_slider)
        self.get_logger().debug(f"Velocity updated to {self.velocity}")

        if joystick_r.btn_bottom_r5_state >= 1:
            self.get_logger().info("Sweeper moving ANTICLOCKWISE")
            self.sweeper_servo.update_direction(self.DIRECTION_ANTICLOCKWISE)
            self.sweeper_servo.update_velocity(self.velocity)
        elif joystick_r.btn_bottom_r6_state >= 1:
            self.get_logger().info("Sweeper moving CLOCKWISE")
            self.sweeper_servo.update_direction(self.DIRECTION_CLOCKWISE)
            self.sweeper_servo.update_velocity(self.velocity)
        else:# Called when the script executes
            self.sweeper_servo.stop()

    def joystick_l(self, joystick_l: InputJoystick):
        """
        Right joystick callback function
        """
        pass

def main():
    rclpy.init()
    node = SweeperNode()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__=="__main__":
    main()
