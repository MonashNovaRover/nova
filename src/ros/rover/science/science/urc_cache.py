#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control for the servos of the URC  drill
caches
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: CacheNode
TOPICS: None
SERVICES:
    - server: /science/cache_command_n
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):	Brandon Chung
CREATION:	03/05/2025
EDITED:		05/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from python_control.controls.OneAxisPositionControl import OneAxisPositionControl
from python_control.controllers.JonoPositionController import JonoPositionController
from nova_interfaces.srv import CacheCommand

class URCCache(Node):
    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    CACHE_SEND_FRAME_PARAM = "frame_id"

    # ROS2 SERVICES
    CACHE_SERVICE = "/science/cache"

    # SENDING COMMAND IDS
    CACHE_MOVE_SERVO = 0x04

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    SERVO_MAX_ANGLE_PARAM = "max_angle"
    SERVO_MAX_ANGLE_DEFAULT = 179
    MAX_VALUE = 0xFF

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

    # All possible commands
    COMMANDS = [
        COM_DEG_0 := (0).to_bytes(1, "big"),
        COM_DEG_90 := (1).to_bytes(1, "big"),
        COM_DEG_180 := (2).to_bytes(1, "big"),
    ]

    def __init__(self):
        super().__init__(name="CacheNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        # Setting ROS parameters
        self.declare_parameter(self.CACHE_SEND_FRAME_PARAM, self.CACHE_SEND_FRAME)

        # Create positions map from params
        self.positions: {str: int} = { k: self.declare_parameter(v, self.POSITION_DEFAULTS[k]).value for k, v in self.POSITION_PARAMS.items() }
        self.get_logger().info(f"POSITIONS: {self.positions}")

        ## Create CONTROLS
        self.cache_servo = OneAxisPositionControl(
            logger=logger,
            max_angle=self.declare_parameter(self.SERVO_MAX_ANGLE_PARAM, self.SERVO_MAX_ANGLE_DEFAULT).value,
            positions = self.positions
        )
        self.cache_servo.update_position(self.DEG_0)

        # Only accept inputs being delivered to receiving CAN IDs
        self.bus.set_id_filter(self.CACHE_SEND_FRAME)

        ## Create SERVICE
        self.command_service = self.create_service(CacheCommand, f'/science/cache_command_{self.CACHE_SEND_FRAME[-1]}', self.command_callback)
        
        ## Start the CAN bus
        self.start_can()

    def command_callback(self, request, response):
        self.cache_servo.set_offset(0)
        match request.angle:
            case self.COM_DEG_0:
                self.cache_servo.update_position(self.DEG_0)
                self.get_logger().info(f"Moved cache to 0 degrees {self.cache_servo.get_goal_position()}")
            case self.COM_DEG_90:
                self.cache_servo.update_position(self.DEG_90)
                self.get_logger().info(f"Moved cache to 90 degrees {self.cache_servo.get_goal_position()}")
            case self.COM_DEG_180:
                self.cache_servo.update_position(self.DEG_180)
                self.get_logger().info(f"Moved cache to 180 degrees {self.cache_servo.get_goal_position()}")
            case _:
                self.get_logger().error(f"Invalid cache command: {request.angle}")
                response.success = False
                return response
        response.success = True
        return response


def main():
    rclpy.init()
    node = URCCache()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
