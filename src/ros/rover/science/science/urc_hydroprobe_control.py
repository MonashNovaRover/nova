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
AUTHOR(S):	Felicity Matthews
CREATION:	01/05/2025
EDITED:		05/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
to run with parameter file:
$ ros2 run science urc_hydroprobe_control.py --ros-args --params-file ~/nova/src/ros/rover/nova_bringup/params/hydroprobe_control.yaml
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
    SERVO_MAX_PERCENT = 1
    SERVO_CONTROL_NAME = "Hydroprobe Control"

    # CONTROL DIRECTIONS
    DIRECTION_UP = Direction.POSITIVE
    DIRECTION_DOWN = Direction.NEGATIVE

    # TUNABLE PARAMETERS
    DEPLOY_SPEED_SEQUENCE = "deploy_speeds"
    DEPLOY_DURATION_SEQUENCE = "deploy_durations"
    RESET_SPEED_SEQUENCE = "reset_speeds"
    RESET_DURATION_SEQUENCE = "reset_durations"
    RETRACT_SPEED_SEQUENCE = "retract_speeds"
    RETRACT_DURATION_SEQUENCE = "retract_durations"

    def __init__(self):
        super().__init__(name="HydroprobeControl", can_bus=self.CAN_BUS, command_period=self.COMMAND_PERIOD)
        logger = self.get_logger()

        self.velocity = 0.5
        self.under_manual_control = False

        self.declare_parameters(
            namespace="",
            parameters=[
                (self.DEPLOY_SPEED_SEQUENCE, [50, 0, -50, 0]),
                (self.DEPLOY_DURATION_SEQUENCE, [1800, 200, 500]),
                (self.RESET_SPEED_SEQUENCE, [-50, 0]),
                (self.RESET_DURATION_SEQUENCE, [4000]),
                (self.RETRACT_SPEED_SEQUENCE, [50, -80, 0, -50, 0]),
                (self.RETRACT_DURATION_SEQUENCE, [500, 2100, 200, 800]),
            ],
        )

        ## Create CONTROLS
        self.hydroprobe_servo = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.SERVO_MAX_PERCENT,
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
        self.add_controller(self.SERVO_CONTROL_NAME, self.hydroprobe_servo_controller)

        # Initialise service for taking commands
        self.command_service = self.create_service(MoveHydroprobe, '/science/move_hydroprobe', self.command_service_callback)

        ## Start the CAN bus
        self.start_can()

        ## Create timers
        self.deploy_timers = self.create_timer(self.get_parameter(self.DEPLOY_SPEED_SEQUENCE).value, self.get_parameter(self.DEPLOY_DURATION_SEQUENCE).value)
        self.reset_timers = self.setup_timers(self.get_parameter(self.RESET_SPEED_SEQUENCE).value, self.get_parameter(self.RESET_DURATION_SEQUENCE).value)
        self.retract_timers = self.create_timer(self.get_parameter(self.RETRACT_SPEED_SEQUENCE).value, self.get_parameter(self.RETRACT_DURATION_SEQUENCE).value)

    def setup_timers(self, speeds, durations):
        """ sets up a sequence of timers """
        timers = []
        if len(speeds) <= len(durations):
            self.get_logger().error("Length of speeds is not greater than length of durations")
            return []

        for i in range(len(durations)):
            timers.append(self.create_timer(
                durations[i] / 1000,
                self.sequence_callback(speeds[i+1], timers, i),
                autostart=False
            ))

        return timers

    def sequence_callback(self, speed, timers, index):
        """ callback function for timers within a sequence """
        def callback():
            timers[index].cancel()
            self.set_movement(speed)
            if len(timers) > index+1:
                timers[index+1].reset()
        return callback

    def set_movement(self, speed):
        """ sets the direction and velocity of the servo """
        if speed > 0:
            self.hydroprobe_servo.update_direction(self.DIRECTION_UP)
        elif speed < 0:
            self.hydroprobe_servo.update_direction(self.DIRECTION_DOWN)

        self.hydroprobe_servo.update_velocity(abs(speed) // 255)

    def reset(self):
        """ starts the reset sequence """
        self.get_logger().info("Hydroprobe RESETTING ...")
        self.set_movement(self.get_parameter(self.RESET_SPEED_SEQUENCE).value[0])
        self.reset_timers[0].reset()

    def deploy(self):
        """ starts the deploy sequence """
        self.get_logger().info("Hydroprobe DEPLOYING ...")
        self.set_movement(self.get_parameter(self.DEPLOY_SPEED_SEQUENCE).value[0])
        self.deploy_timers[0].reset()

    def retract(self):
        """ starts the retract sequence """
        self.get_logger().info("Hydroprobe RETRACTING ...")
        self.set_movement(self.get_parameter(self.RETRACT_SPEED_SEQUENCE).value[0])
        self.retract_timers[0].reset()

    def command_service_callback(self, request, response):
        """ callback for the MoveHydroprobe service"""
        response.success = True

        match request.command:
            case MoveHydroprobe.RESET:
                self.reset()
            case MoveHydroprobe.MOVE_UP:
                self.retract()
            case MoveHydroprobe.MOVE_DOWN:
                self.deploy()
            case _:
                self.get_logger().error(f"Invalid hydroprobe command: {request.command}")
                response.success = False

        return response

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
            self.under_manual_control = True
        elif joystick_r.btn_bottom_r6_state >= 1:
            self.get_logger().info("Hydroprobe moving DOWN")
            self.hydroprobe_servo.update_direction(self.DIRECTION_UP)
            self.hydroprobe_servo.update_velocity(self.velocity)
            self.under_manual_control = True
        elif self.under_manual_control:
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
