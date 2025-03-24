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
    SERVO_MAX_ANGLE_PARAM = "max_angle"
    SERVO_MAX_ANGLE_DEFAULT = 239
    MAX_VALUE = 0xFF

    SPIN_CONTROL_NAME = "Analysis Arm Spinny Part"

    # Positions
    POSITION_NAMES = [
        SWEEPER := "sweeper",
        MICROSCOPE := "microscope",
        NIR_PROBE := "nir_probe",
    ]

    # New positions after testing (in CAN message units)
    #   - 0x00 (sweeper)
    #   - 0x80 (microscope)
    #   - 0xFF (nir probe)
    POSITION_DEFAULTS = {
        SWEEPER: 0,
        MICROSCOPE: 120,
        NIR_PROBE: 239,
    }

    POSITION_PARAMS = {
        SWEEPER: SWEEPER + "_pos",
        MICROSCOPE: MICROSCOPE + "_pos",
        NIR_PROBE: NIR_PROBE + "_pos",
    }

    # Offset variables
    OFFSET_STEP_DEFAULT = 1
    OFFSET_STEP_PARAM = "offset_step"

    def __init__(self):
        super().__init__(name="SpinnyPartNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.declare_parameter(self.OFFSET_STEP_PARAM, self.OFFSET_STEP_DEFAULT)

        # Create positions map from params
        self.positions = { k: self.declare_parameter(v, self.POSITION_DEFAULTS[k]) for k, v in self.POSITION_PARAMS }

        ## Create CONTROLS
        self.spinny_part = OneAxisPositionControl(
            logger=logger,
            max_angle=self.declare_parameter(self.SERVO_MAX_ANGLE_PARAM, self.SERVO_MAX_ANGLE_DEFAULT).value,
            positions = self.positions
        )
        self.spinny_part.update_position(self.NIR_PROBE)

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
        self.get_logger().info(f"POSITIONS: {self.POSITIONS}")

        ## Start the CAN bus
        self.start_can()

    def joystick_l(self, joystick_l: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        # change position
        if joystick_l.btn_bottom_r4_state == 1:
            self.spinny_part.set_offset(0)
            self.spinny_part.update_position(self.SWEEPER)
            self.get_logger().info(f"Moved to SWEEPER position {self.POSITIONS[self.SWEEPER] + self.spinny_part.get_offset()}")
        elif joystick_l.btn_bottom_r5_state == 1:
            self.spinny_part.set_offset(0)
            self.spinny_part.update_position(self.MICROSCOPE)
            self.get_logger().info(f"Moved to MICROSCOPE position {self.POSITIONS[self.MICROSCOPE] + self.spinny_part.get_offset()}")
        elif joystick_l.btn_bottom_r6_state == 1:
            self.spinny_part.set_offset(0)
            self.spinny_part.update_position(self.NIR_PROBE)
            self.get_logger().info(f"Moved to NIR PROBE position {self.POSITIONS[self.NIR_PROBE] + self.spinny_part.get_offset()}")

        # twitch/update offset
        if joystick_l.btn_bottom_r1_state == 1:
            self.spinny_part.set_offset(self.spinny_part.get_offset() + self.get_parameter(self.OFFSET_STEP_PARAM).value)
            self.get_logger().info(f"OFFSET updated {self.spinny_part.get_offset()}")
        elif joystick_l.btn_bottom_r2_state == 1:
            self.spinny_part.set_offset(self.spinny_part.get_offset() - self.get_parameter(self.OFFSET_STEP_PARAM).value)
            self.get_logger().info(f"OFFSET updated {self.spinny_part.get_offset()}")

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
