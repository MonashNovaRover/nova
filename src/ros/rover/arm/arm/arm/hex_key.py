#!/usr/bin/env python3

import logging
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.ControllerNode import ControllerNode
from python_control.controls.Direction import Direction
from python_control.controllers.JonoVelocityController import JonoVelocityController
import rclpy
from input_interfaces.msg import InputJoystick

class HexKey(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    HEX_KEY_SEND = 0x0E1

    # NAMES
    # Add any CONTROL names here
    HEX_KEY_CONTROL = "hex_key"

    # CONTROL PARAMETERS
    # Directions
    HEX_KEY_CLOCKWISE = Direction.POSITIVE
    HEX_KEY_COUNTERCLOCKWISE = Direction.NEGATIVE

    # SENDING COMMAND IDS
    # Add any CONTROL command ids here
    HEX_KEY_SEND_CLOCKWISE = 0x00
    HEX_KEY_SEND_COUNTERCLOCKWISE = 0x01

    def __init__(self):
        super().__init__(name="HexKey", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        # Flags
        self.thumb_stick_y_pressed = False

        ## Create controls
        self.hex_key_control = OneAxisVelocityControl(
            logger=logger,
            max_percent=0.5,
        )

        ## Create controllers
        self.sample_tray_controller = JonoVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.HEX_KEY_SEND,
            pos_command=self.HEX_KEY_SEND_CLOCKWISE,
            neg_command=self.HEX_KEY_SEND_COUNTERCLOCKWISE,
            control=self.hex_key_control,
        )

        ## Add controllers
        self.add_controller(self.HEX_KEY_CONTROL, self.sample_tray_controller)

        ## Start the CAN bus
        self.start_can()

    def update_hex_key_speed(self, joystick_l: InputJoystick):
        control = self.hex_key_control

        if joystick_l.ax_thumb_y == 0:
            self.thumb_stick_y_pressed = False

        if joystick_l.ax_thumb_y == 1 and not self.thumb_stick_y_pressed:
            control.update_max_percent(round(control.get_max_percent() + 0.1, 1))
            self.thumb_stick_y_pressed = True
            self.get_logger().info(f"Max Percent Increased: {control.get_max_percent()}")
        elif joystick_l.ax_thumb_y == -1 and not self.thumb_stick_y_pressed:
            control.update_max_percent(round(control.get_max_percent() - 0.1, 1))
            self.thumb_stick_y_pressed = True
            self.get_logger().info(f"Max Percent Decreased: {control.get_max_percent()}")

    def update_hex_key(self, joystick_r: InputJoystick):
        control = self.hex_key_control

        if joystick_r.ax_thumb_y == 1:
            control.update_direction(self.HEX_KEY_CLOCKWISE)
            control.update_velocity(control.get_max_percent())
        elif joystick_r.ax_thumb_y == -1:
            control.update_direction(self.HEX_KEY_COUNTERCLOCKWISE)
            control.update_velocity(control.get_max_percent())
        else:
            control.stop()


    def joystick_l(self, joystick_l: InputJoystick):
        self.update_hex_key_speed(joystick_l)

    def joystick_r(self, joystick_r: InputJoystick):
        self.update_hex_key(joystick_r)

def main():
    rclpy.init()
    node = HexKey()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()