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
CREATION:	23/04/2025
EDITED:		23/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from std_srvs.srv import SetBool

from python_control.ControllerNode import ControllerNode
from python_control.controllers.ToggleController import ToggleController
from python_control.controls.ToggleControl import ToggleControl


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
    # Add any CONTROL command ids here
    CACHE_COMMANDS = [
        CACHE1_SEND_OPEN := 0x05,
        CACHE1_SEND_CLOSE := 0x06,
        CACHE2_SEND_CLOSE := 0x07,
        CACHE2_SEND_CLOSE := 0x08,
    ]

    def __init__(self):
        super().__init__(name="CacheNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        # Initialise 2 seperate cache controls
        self.cache_control1 = ToggleControl(logger=logger, on=False)
        self.cache_controller1 = ToggleController(
            logger=logger,
            bus=self.bus,
            frame_id=self.CACHE_SEND_FRAME,
            toggle_command_on=self.CACHE_SEND_OPEN,
            toggle_command_off=self.CACHE_SEND_CLOSE,
            control=self.cache_control1,
        )

        self.cache_control2 = ToggleControl(logger=logger, on=False)
        self.cache_controller2 = ToggleController(
            logger=logger,
            bus=self.bus,
            frame_id=self.CACHE_SEND_FRAME,
            toggle_command_on=self.CACHE_SEND_OPEN,
            toggle_command_off=self.CACHE_SEND_CLOSE,
            control=self.cache_control2,
        )


        self.start_can()

        self.create_service(SetBool, self.CACHE_SERVICE, self.cache_callback)

    def cache_callback(
        self,
        request: SetBool.Request,
        response: SetBool.Response,
    ) -> SetBool.Response:
        if request.data:
            self.cache_control.start()
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
