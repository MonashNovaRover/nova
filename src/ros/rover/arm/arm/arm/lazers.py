#!/usr/bin/env python3

import logging
from python_control.ControllerNode import ControllerNode
from python_control.controls.Direction import Direction
from python_control.controllers.ToggleController import ToggleController
from python_control.controls.ToggleControl import ToggleControl
import rclpy
from input_interfaces.msg import InputJoystick

class Lazers(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    LAZERS_SEND_FRAME = 0x0E3

    # NAMES
    # Add any CONTROL names here
    LAZERS_CONTROL = "lazers"

    # SENDING COMMAND IDS
    # Add any CONTROL command ids here
    LAZER_SEND_CONTROL_ID = 0x00
    LAZER_SEND_ON = 0x00


    def __init__(self):
        super().__init__(name="Lazers", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        ## Create controls
        self.lazers_control = ToggleControl(
            logger=logger,
            on=False,
        )

        ## Create controllers
        self.lazers_controller = ToggleController(
            logger=logger,
            bus=self.bus,
            frame_id=self.LAZERS_SEND_FRAME,
            control_id=self.LAZER_SEND_CONTROL_ID,
            toggle_command_on=self.LAZER_SEND_ON,
            control=self.lazers_control,
        )

        ## Add controllers
        self.add_controller(self.LAZERS_CONTROL, self.lazers_controller)

        ## Start the CAN bus
        self.start_can()

    def update_lazers(self, joystick_l: InputJoystick):
        if joystick_l.btn_thumb_d_state == 1:
            self.lazers_control.toggle()
            self.get_logger().info("Lazers {0}".format("ON" if self.lazers_control.is_on() else "OFF"))
          

    def joystick_l(self, joystick_l: InputJoystick):
        self.update_lazers(joystick_l)

    def joystick_r(self, joystick_r: InputJoystick):
        pass

def main():
    rclpy.init()
    node = Lazers()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()