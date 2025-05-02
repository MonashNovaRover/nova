#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control for the URC Hydroprobe actuation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: HydroprobeControlNode
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
from nova_interfaces.srv import MoveHydroprobe


class HydroprobeControlNode(JoystickControllerNode):
    # CAN BUS NAME
    CAN_BUS = "can1"

    # SENDING CARD IDS
    SERVO_ID = 0x0A0
    COMMAND_PERIOD = 0.3

    # SENDING COMMAND IDS
    MOVE_UP = 0x01
    MOVE_DOWN = 0x02

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    SWEEPER_MAX_PERCENT = 1
    SWEEPER_CONTROL_NAME = "Hydroprobe Control"

    # CONTROL DIRECTIONS
    DIRECTION_UP = Direction.POSITIVE
    DIRECTION_DOWN = Direction.NEGATIVE

    # TUNABLE PARAMETERS
    SPEED_DEPLOY = "speed_deploy"
    SPEED_RESET = "speed_reset"
    SPEED_RETRACT = "speed_retract"
    DURATION_RESET = "duration_reset"
    DURATION_DEPLOY = "duration_deploy"
    DURATION_RETRACT_DOWN = "duration_retract_down"
    DURATION_RETRACT_UP = "duration_retract_up"

    def __init__(self):
        super().__init__(name="SweepyServoNode", can_bus=self.CAN_BUS, command_period=self.COMMAND_PERIOD)
        logger = self.get_logger()

        self.velocity = 0.5

        self.declare_parameters(
            namespace="",
            parameters=[
                (self.SPEED_DEPLOY, 10),
                (self.SPEED_RESET, 108),
                (self.SPEED_RETRACT, 175),
                (self.DURATION_RESET, 4),
                (self.DURATION_DEPLOY, 1.8),
                (self.DURATION_RETRACT_UP, 0.5),
                (self.DURATION_RETRACT_UP, 2.1),
            ],
        )

        ## Create CONTROLS
        self.hydroprobe_servo = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.SWEEPER_MAX_PERCENT,
        )

        ## Create CONTROLLERS
        self.hydroprobe_servo_controller = JonoVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.SERVO_ID,
            pos_command=self.MOVE_UP,
            neg_command=self.MOVE_DOWN,
            control=self.hydroprobe_servo,
        )

        ## Add the CONTROLLERS to the node's controllers
        self.add_controller(self.SWEEPER_CONTROL_NAME, self.hydroprobe_servo_controller)

        # Initialise service for taking commands
        self.command_service = self.create_service(MoveHydroprobe, '/science/move_hydroprobe', self.command_service_callback)

        ## Start the CAN bus
        self.start_can()

    def reset(self):
        self.get_logger().info("Hydroprobe RESETTING")

    def move_down(self):
        self.get_logger().info("Hydroprobe MOVING DOWN")

    def move_up(self):
        self.get_logger().info("Hydroprobe MOVING UP")

    def command_service_callback(self, request, response):
        match request.command:
            case MoveHydroprobe.RESET:
                self.reset()
            case MoveHydroprobe.MOVE_UP:
                self.move_up()
            case MoveHydroprobe.MOVE_DOWN:
                self.move_down()
            case _:
                self.get_logger().error(f"invalid hydroprobe command {request.command}")

    def joystick_r(self, joystick_r: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.velocity = abs(joystick_r.ax_slider)
        self.get_logger().debug(f"Velocity updated to {self.velocity}")

        if joystick_r.btn_bottom_r3_state >= 1:
            self.get_logger().info("Hydroprobe moving UP")
            self.hydroprobe_servo.update_direction(self.DIRECTION_DOWN)
            self.hydroprobe_servo.update_velocity(self.velocity)
        elif joystick_r.btn_bottom_r6_state >= 1:
            self.get_logger().info("Hydroprobe moving DOWN")
            self.hydroprobe_servo.update_direction(self.DIRECTION_UP)
            self.hydroprobe_servo.update_velocity(self.velocity)
        else:# Called when the script executes
            self.hydroprobe_servo.stop()

    def joystick_l(self, joystick_l: InputJoystick):
        """
        Right joystick callback function
        """
        pass

def main():
    rclpy.init()
    node = HydroprobeControlNode()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__=="__main__":
    main()
