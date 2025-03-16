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
from input_interfaces.msg import InputJoystick
from python_control.controls.OneAxisPositionControl import OneAxisPositionControl
from python_control.controllers.JonoPositionController import JonoPositionController


class SpinnyPartNode(JoystickControllerNode):
    # CAN BUS NAME
    CAN_BUS = "can1"

    # SENDING CARD IDS
    SERVO_ID = 0x0A0

    # SENDING COMMAND IDS
    MOVE_SERVO_COMMAND = 0x04

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    SERVO_MAX_ANGLE = 360
    MAX_VALUE = 0xFF

    SPIN_CONTROL_NAME = "Analysis Arm Spinny Part"

    # Positions
    POSITION_NAMES = [
        MICROSCOPE := "microscope",
        SWEEPER := "sweeper",
        NIR_PROBE := "nir_probe",
    ]

    POSITIONS = {
        MICROSCOPE: 60,
        SWEEPER: 180,
        NIR_PROBE: 300,
    }

    # Offset variables
    MAX_OFFSET = 60
    MIN_OFFSET = -60

    def __init__(self):
        super().__init__(name="SpinnyPartNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.offset = 0

        ## Create CONTROLS
        self.spinny_part = OneAxisPositionControl(
            logger=logger,
            max_angle=self.SERVO_MAX_ANGLE,
            positions = self.POSITIONS
        )

        ## Create CONTROLLERS
        self.spinny_part_controller = JonoPositionController(
            logger=logger,
            bus=self.bus,
            pos_command=self.MOVE_SERVO_COMMAND,
            frame_id=self.SERVO_ID,
            control=self.spinny_part,
            max_value=self.MAX_VALUE
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
        # change position
        if joystick_l.btn_thumb_l_state == 1:
            self.spinny_part.update_position(self.MICROSCOPE)
        elif joystick_l.btn_thumb_d_state == 1:
            self.spinny_part.update_position(self.SWEEPER)
        elif joystick_l.btn_thumb_r_state == 1:
            self.spinny_part.update_position(self.NIR_PROBE)

        # twitch/update offset
        if joystick_l.btn_bottom_r1_state == 1:
            self.spinny_part.set_offset(min(self.MAX_OFFSET, self.spinny_part.get_offset() + 1))
        elif joystick_l.btn_bottom_r2_state == 1:
            self.spinny_part.set_offset(max(self.MIN_OFFSET, self.spinny_part.get_offset() - 1))

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
