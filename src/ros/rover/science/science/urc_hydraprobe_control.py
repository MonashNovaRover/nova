#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control for the URC Hydraprobe actuation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: HydraprobeControlNode
TOPICS:
  - subscriber: /inputs/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):	Felicity Matthews
CREATION:	01/05/2025
EDITED:		05/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
to run with parameter file:
$ ros2 run science urc_hydraprobe_control.py --ros-args --params-file ~/nova/src/ros/rover/nova_bringup/params/hydraprobe_control.yaml
"""

import rclpy
from python_control.controls.Direction import Direction
from input_interfaces.msg import InputJoystick
from python_control.JoystickControllerNode import JoystickControllerNode
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controllers.JonoVelocityController import JonoVelocityController
from nova_interfaces.srv import MoveHydraprobe

class HydraprobeControlNode(JoystickControllerNode):
    # CAN BUS NAME
    CAN_BUS = "can1"

    # SENDING CARD IDS
    SERVO_ID = 0x0A0
    COMMAND_PERIOD = 0.3

    # SENDING COMMAND IDS
    MOVE_UP = 0x06
    MOVE_DOWN = 0x07

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    SERVO_MAX_PERCENT = 1
    SERVO_CONTROL_NAME = "hydraprobe Control"

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

    # REQUEST COMMANDS
    COMMANDS = [
        RESET_COMMAND := (0).to_bytes(1, "big"), # Reset the hydraprobe to resting location
        RETRACT_COMMAND := (1).to_bytes(1, "big"), # Retract the hydraprobe from deployed to resting location
        DEPLOY_COMMAND := (2).to_bytes(1, "big"), # Deploy the hydraprobe from resting location
    ]

    def __init__(self):
        super().__init__(name="hydraprobeControl", can_bus=self.CAN_BUS, command_period=self.COMMAND_PERIOD)
        logger = self.get_logger()

        self.velocity = 0.5
        self.under_manual_control = False
        self.moving = False

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
        self.hydraprobe_servo = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.SERVO_MAX_PERCENT,
        )

        ## Create CONTROLLERS
        self.hydraprobe_servo_controller = JonoVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.SERVO_ID,
            pos_command=self.MOVE_UP,
            neg_command=self.MOVE_DOWN,
            control=self.hydraprobe_servo,
        )

        ## Add the CONTROLLERS to the node's controllers
        self.add_controller(self.SERVO_CONTROL_NAME, self.hydraprobe_servo_controller)

        # Initialise service for taking commands
        self.command_service = self.create_service(MoveHydraprobe, '/science/move_hydraprobe', self.command_service_callback)

        ## Start the CAN bus
        self.start_can()

        ## Create timers
        self.deploy_timers = self.setup_timers(self.get_parameter(self.DEPLOY_SPEED_SEQUENCE).value, self.get_parameter(self.DEPLOY_DURATION_SEQUENCE).value)
        self.reset_timers = self.setup_timers(self.get_parameter(self.RESET_SPEED_SEQUENCE).value, self.get_parameter(self.RESET_DURATION_SEQUENCE).value)
        self.retract_timers = self.setup_timers(self.get_parameter(self.RETRACT_SPEED_SEQUENCE).value, self.get_parameter(self.RETRACT_DURATION_SEQUENCE).value)

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
            self.get_logger().info(f"timer {index} callback activated")
            timers[index].cancel()
            self.set_movement(speed)
            if len(timers) > index+1:
                timers[index+1].reset()
            else:
                self.get_logger().info("sequence completed")
                self.moving = False
        return callback

    def set_movement(self, speed):
        """ sets the direction and velocity of the servo """
        if speed > 0:
            self.hydraprobe_servo.update_direction(self.DIRECTION_UP)
        elif speed < 0:
            self.hydraprobe_servo.update_direction(self.DIRECTION_DOWN)

        self.get_logger().info(f"Speed: {speed} | Velocity: {abs(speed) / 255}")
        self.hydraprobe_servo.update_velocity(abs(speed) / 255)

    def reset(self):
        """ starts the reset sequence """
        self.get_logger().info("hydraprobe RESETTING ...")
        self.moving = True
        self.set_movement(self.get_parameter(self.RESET_SPEED_SEQUENCE).value[0])
        self.reset_timers[0].reset()

    def deploy(self):
        """ starts the deploy sequence """
        self.get_logger().info("hydraprobe DEPLOYING ...")
        self.moving = True
        self.set_movement(self.get_parameter(self.DEPLOY_SPEED_SEQUENCE).value[0])
        self.deploy_timers[0].reset()

    def retract(self):
        """ starts the retract sequence """
        self.get_logger().info("hydraprobe RETRACTING ...")
        self.moving = True
        self.set_movement(self.get_parameter(self.RETRACT_SPEED_SEQUENCE).value[0])
        self.retract_timers[0].reset()

    def command_service_callback(self, request, response):
        """ callback for the MoveHydraprobe service"""
        self.get_logger().info("responding to hydraprobe service call...")

        if self.moving:
            self.get_logger().info("hydraprobe is already moving")
            response.success = False
            return response

        response.success = True
        match request.command:
            case self.RESET_COMMAND:
                self.reset()
            case self.RETRACT_COMMAND:
                self.retract()
            case self.DEPLOY_COMMAND:
                self.deploy()
            case _:
                self.get_logger().error(f"Invalid hydraprobe command: {request.command}")
                response.success = False

        return response

    def joystick_r(self, joystick_r: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        if self.moving:
            return

        self.velocity = abs(joystick_r.ax_slider)
        self.get_logger().debug(f"Velocity updated to {self.velocity}")

        if joystick_r.btn_bottom_r3_state >= 1:
            self.get_logger().info("hydraprobe moving UP")
            self.hydraprobe_servo.update_direction(self.DIRECTION_DOWN)
            self.hydraprobe_servo.update_velocity(self.velocity)
            self.under_manual_control = True
        elif joystick_r.btn_bottom_r6_state >= 1:
            self.get_logger().info("hydraprobe moving DOWN")
            self.hydraprobe_servo.update_direction(self.DIRECTION_UP)
            self.hydraprobe_servo.update_velocity(self.velocity)
            self.under_manual_control = True
        elif self.under_manual_control:
            self.hydraprobe_servo.stop()

    def joystick_l(self, joystick_l: InputJoystick):
        """
        Right joystick callback function
        """
        pass

def main():
    rclpy.init()
    node = HydraprobeControlNode()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__=="__main__":
    main()
