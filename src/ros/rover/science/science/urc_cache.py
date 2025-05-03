#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control for the servos of the URC  drill
caches
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: CacheNode
TOPICS:
  - subscriber: /inputs/input_joystick_l [InputJoystick]
  - subscriber: /inputs/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):	Brandon Chung
CREATION:	03/05/2025
EDITED:		03/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from python_control.JoystickControllerNode import JoystickControllerNode
from input_interfaces.msg import InputJoystick
from python_control.controls.OneAxisPositionControl import OneAxisPositionControl
from python_control.controllers.JonoPositionController import JonoPositionController


class URCCache(ControllerNode):
    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    CACHE_SEND_FRAME = 0x060

    # ROS2 SERVICES
    CACHE_SERVICE = "/science/cache"

    # SENDING COMMAND IDS
    CACHE_MOVE_SERVO = 0x04

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    SERVO_MAX_ANGLE_PARAM = "max_angle"
    SERVO_MAX_ANGLE_DEFAULT = 179
    MAX_VALUE = 0xFF

    SPIN_CONTROL_NAME = "Drill Cache Doors"

    # Positions
    POSITION_NAMES = [
        DEG_0 := "0_degrees",
        DEG_90 := "90_degrees",
        DEG_180 := "180_degrees",
    ]

    # New positions after testing (in CAN message units)
    #   - 0x00 (0 degrees)
    #   - 0x80 (90 degrees)
    #   - 0xFF (179 degrees)
    POSITION_DEFAULTS = {
        DEG_0: 0,
        DEG_90: 90,
        DEG_180: 179,
    }

    POSITION_PARAMS = {
        DEG_0: DEG_0 + "_pos",
        DEG_90: DEG_90 + "_pos",
        DEG_180: DEG_180 + "_pos",
    }

    # Offset variables
    OFFSET_STEP_DEFAULT = 5
    OFFSET_MAX_STEP_DEFAULT = 30
    OFFSET_MAX_STEP_PARAM = "step_max"

    def __init__(self):
        super().__init__(name="CacheNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.offset_step = self.OFFSET_STEP_DEFAULT
        self.declare_parameter(self.OFFSET_MAX_STEP_PARAM, self.OFFSET_MAX_STEP_DEFAULT)
        logger.info(f"Max offset step: {self.get_parameter(self.OFFSET_MAX_STEP_PARAM).value} | Current offset step: {self.offset_step}")

        # Create positions map from params
        self.positions: {str: int} = { k: self.declare_parameter(v, self.POSITION_DEFAULTS[k]).value for k, v in self.POSITION_PARAMS.items() }

        ## Create CONTROLS
        self.cache_servo = OneAxisPositionControl(
            logger=logger,
            max_angle=self.declare_parameter(self.SERVO_MAX_ANGLE_PARAM, self.SERVO_MAX_ANGLE_DEFAULT).value,
            positions = self.positions
        )
        self.cache_servo.update_position(self.DEG_0)

        ## Create CONTROLLERS
        self.cache_servo_controller = JonoPositionController(
            logger=logger,
            bus=self.bus,
            pos_command=self.CACHE_MOVE_SERVO,
            frame_id=self.SERVO_ID,
            control=self.cache_servo,
            max_value=self.MAX_VALUE
        )

        ## Add the CONTROLLERS to the node's controllers
        self.add_controller(self.SPIN_CONTROL_NAME, self.cache_servo_controller)
        self.get_logger().info(f"POSITIONS: {self.positions}")

        ## Start the CAN bus
        self.start_can()

    def cache_callback(
        self,
        request: SetBool.Request,
        response: SetBool.Response,
    ) -> SetBool.Response:
        if request.data:
            self.spinny_part.set_offset(0)
            self.spinny_part.update_position(self.MICROSCOPE)
            self.get_logger().info(f"Moved cache to 0 degrees {self.spinny_part.get_goal_position()}")
        else:
            self.cache_control.stop()
        self.cache_controller.control_send_callback()
        response.success = True
        return response


def main():
    rclpy.init()
    node = URCCache()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
