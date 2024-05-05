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
INSTRUCTIONS:
- This is a template for creating new control nodes
- Replace all instances of "TemplateNode" with the name of the new node
- Replace all instances of "CONTROL_NAME" with the name of the new control
- Remove any uneeded classes (Sensor, Limit, Control, Controller, etc.)
- Add any new classes as needed (Sensor, Limit, Control, Controller, etc.)
- Remove any unused imports
- Remove helper comments and replace with useful information
"""

import rclpy
from python_control.ControllerNode import ControllerNode
from python_control.controllers.CMDVelocityController import CMDVelocityController
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controls.Direction import Direction
from python_control.sensors.RangeSensor import RangeSensor
from python_control.limits.IntegerLimit import IntegerLimit
from python_control.limits.LimitSwitchLimit import LimitSwitchLimit
from python_control.sensors.LimitSwitchSensor import LimitSwitchSensor
from input_interfaces.msg import InputJoystick
from sensor_msgs.msg import Range


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

    # LIMIT PARAMETERS
    # Add any LIMIT parameters here
    TIME_OF_FLIGHT_OFFSET = 20
    TIME_OF_FLIGHT_MIN = 30
    TIME_OF_FLIGHT_MAX = 165


    def __init__(self):
        """
        Node Initialization
        Define all the sensors, limits, controls, and controllers here
        """
        super().__init__(name="TemplateNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        ## Add FLAGS as required
        # To be used in the joystick callback functions
        self.control_enable = True

        ## Add RECIEVE CAN IDS to the bus filter
        self.bus.set_id_filter([self.TOF_FRAME_ID, self.LIMIT_SWITCH_FRAME_ID])

        ## Add PUBLISHERS to the node
        self.tof_publisher = self.create_publisher(Range, "/example/tof", 10)

        ## Create SENSORS
        tof_sensor = RangeSensor(
            logger=logger,
            bus=self.bus,
            frame_id=self.TOF_FRAME_ID,
            maximum=self.TIME_OF_FLIGHT_MAX,
            minimum=self.TIME_OF_FLIGHT_MIN,
            offset=self.TIME_OF_FLIGHT_OFFSET,
            publisher=self.tof_publisher,
            run_can=False, # Set to False when using Sensor as in a Limit
        )

        limit_switch_top = LimitSwitchSensor(
            logger=logger,
            bus=self.bus,
            frame_id=self.LIMIT_SWITCH_FRAME_ID,
            command_id=self.LIMIT_SWITCH_COMMAND_ID,
            run_can=False, # Set to False when using Sensor as in a Limit
        )

        ## Create LIMITS
        platform_bottom_limit = IntegerLimit(
            logger=logger,
            bus=self.bus,
            maximum=False,
            limit_value=self.TIME_OF_FLIGHT_MIN,
            integer_sensor=tof_sensor,
        )

        platform_top_limit = LimitSwitchLimit(
            logger=logger,
            bus=self.bus,
            limit_switch=limit_switch_top,
        )

        ## Create CONTROLS
        self.control_name = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.CONTROL_NAME_MAX_PERCENT,
            pos_limit=platform_bottom_limit, # pos limit is optional
            neg_limit=platform_top_limit, # neg limit is optional
        )

        ## Create CONTROLLERS
        self.control_name_controller = CMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.CMD_ID,
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