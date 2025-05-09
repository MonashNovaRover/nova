#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
Handles the RGB values sent over ROS to the canbus.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: led_strip
SERVICES:
    - /nova_interfaces/srv/RGBInput.srv          [server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        electronics
AUTHOR(S):      Matthew Hee, Kuhu Tosniwal
CREATION:       18/03/2025
EDITED:         18/03/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy, jcan
import time
from python_control.ControllerNode import ControllerNode
from nova_interfaces.srv import RGBInput

class LedStrip(ControllerNode):

    CAN_BUS = "can0"

#     RED_CONTROL_ID = 0x092
#     GREEN_CONTROL_ID = 0x093
#     BLUE_CONTROL_ID = 0x094

    COLOR_ID = 0X096

    def __init__(self):
        super(LedStrip, self).__init__(name="led_strip", can_bus=self.CAN_BUS)
        self.led_service = self.create_service(RGBInput, '/set_RGBInput', self.led_service_callback)
        self.flash_timer = None
        self.flash_on = False
        self.flash_rgb = (0, 0, 0)
        self.start_can()

    def led_service_callback(self, request, response):
        self.get_logger().info(f"Received service request: {request}")

        if request.flash:
            # Store the chosen flash RGB values
            self.flash_rgb = (request.r, request.g, request.b)
            self.flash_on = False  # Start with LEDs off

            # Cancel existing flash timer if active
            if self.flash_timer:
                self.flash_timer.cancel()

            # Create new timer that toggles every 0.5 seconds
            self.flash_timer = self.create_timer(0.5, self.flash_led_callback)
            self.get_logger().info("Started LED flashing.")
            response.success = True

        else:
            # Cancel flashing if it's active
            if self.flash_timer:
                self.flash_timer.cancel()
                self.flash_timer = None
                self.get_logger().info("Stopped LED flashing.")

            # Set the LEDs to the requested static color
            response.success = self.set_rgb(request.r, request.g, request.b)

        return response

    def send_can_message(self, frame_id, data):
        frame = jcan.Frame(frame_id, data)
        try:
            self.get_logger().info(f"Sending {frame}")
            self.bus.send(frame)
        except Exception as e:
            self.get_logger().error(f"Failed to send CAN message: {e}")
            return False
        return True

    def set_rgb(self, r, g, b):
        red = (r // 16) << 4
        green = g // 16
        blue = (b // 16) << 4
        data = [red + green, blue]
        return self.send_can_message(self.COLOR_ID, data)

    def flash_led_callback(self):
        if self.flash_on:
            self.set_rgb(0, 0, 0)
            self.flash_on = False
        else:
            r, g, b = self.flash_rgb
            self.set_rgb(r, g, b)
            self.flash_on = True


def main(args=None):
    rclpy.init(args=args)
    node = LedStrip()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()