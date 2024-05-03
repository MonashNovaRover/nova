#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: TemplateNode for creating new control nodes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: TemplateNode
TOPICS:
  - subscriber: /inputs/input_joystick_l [InputJoystick]
  - subscriber: /inputs/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    pyton_control
AUTHOR(S):	Tristan Clark
CREATION:	03/05/2024
EDITED:		03/05/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from python_control.ControllerNode import ControllerNode
from python_control.cards.CMDCardController import CMDCardController
from python_control.controls.OneAxisControl import Direction, OneAxisControl
from python_control.sensors.IntegerSensor import IntegerSensor
from python_control.limits.IntegerLimit import IntegerLimit
from python_control.limits.LimitSwitchLimit import LimitSwitchLimit
from python_control.sensors.LimitSwitchSensor import LimitSwitchSensor
from input_interfaces.msg import InputJoystick


class TemplateNode(ControllerNode):
    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    CMD_ID = 0x033

    # RECEIVING CARD IDS
    # Add any SENSOR FRAME / CARD IDS here
    TOF_FRAME_ID = 0x4A1 
    LIMIT_SWITCH_FRAME_ID = 0x4A2

    # CONTROL NAMES
    # Add any CONTROL names here
    CONTROL_NAME = "control_name"

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    CONTROL_NAME_MAX_PERCENT = 0.5

    # SENDING COMMAND IDS
    # Add any CONTROL command ids here

    # RECEIVING COMMAND IDS
    # Add any SENSOR command ids here
    LIMIT_SWITCH_COMMAND_ID = 0x01

    # CONTROL DIRECTIONS
    # Add any CONTROL DIRECTIONS here
    CONTROL_NAME_UP = Direction.NEGATIVE
    CONTROL_NAME_DOWN = Direction.POSITIVE


    def __init__(self):
        """
        Node Initialization
        Define all the sensors, limits, controls, and controllers here
        """
        super().__init__(name="TemplateNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        ## Add RECIEVE CAN IDS to the bus filter
        self.bus.set_id_filter([self.TOF_FRAME_ID, self.LIMIT_SWITCH_FRAME_ID])

        ## Create SENSORS
        tof_sensor = IntegerSensor(
            logger=logger,
            bus=self.bus,
            frame_id=self.TOF_FRAME_ID,
        )

        limit_switch_top = LimitSwitchSensor(
            logger=logger,
            bus=self.bus,
            frame_id=self.LIMIT_SWITCH_FRAME_ID,
            command_id=self.LIMIT_SWITCH_COMMAND_ID,
        )

        ## Create LIMITS
        platform_bottom_limit = IntegerLimit(
            logger=logger,
            bus=self.bus,
            maximum=False,
            limit_value=10,
            integer_sensor=tof_sensor
        )

        platform_top_limit = LimitSwitchLimit(
            logger=logger,
            bus=self.bus,
            limit_switch=limit_switch_top
        )

        ## Create CONTROLS
        self.control_name = OneAxisControl(
            max_percent=self.CONTROL_NAME_MAX_PERCENT,
            pos_limit=platform_bottom_limit, # pos limit is optional
            neg_limit=platform_top_limit, # neg limit is optional
        )

        ## Create CONTROLLERS
        self.control_name_controller = CMDCardController(
            logger=logger,
            bus=self.bus,
            card_id=self.CMD_ID,
            control=self.control_name,
        )

        ## Add the CONTROLLERS to the node's controllers
        self.add_controller(self.CONTROL_NAME, self.control_name_controller)

        ## Start the CAN bus
        self.start_can()


    def joystick_l(self, joystick_l: InputJoystick):
        """
        Left joystick callback function
        Define all the control actions for the left joystick
        NOTE: Joystick lock is handled in the ControllerNode
        """
        pass

    def joystick_r(self, joystick_r: InputJoystick):
        """
        Right joystick callback function
        Define all the control actions for the right joystick
        NOTE: Joystick lock is handled in the ControllerNode
        """
        pass

def main():
    rclpy.init()
    node = TemplateNode()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()