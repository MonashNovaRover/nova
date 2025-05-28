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

    RED_CONTROL_ID = 0x092
    GREEN_CONTROL_ID = 0x093
    BLUE_CONTROL_ID = 0x094

    def __init__(self):
        super(LedStrip, self).__init__(name="led_strip", can_bus=self.CAN_BUS)
        self.led_service = self.create_service(RGBInput, '/set_RGBInput', self.led_service_callback)

        #Timers
        self.green_timer = self.create_timer(0.1, self.set_green, autostart=False)
        self.blue_timer = self.create_timer(0.2, self.set_blue, autostart=False)
        self.flash_timer = None

        self.last_green = 0
        self.last_blue = 0

        self.flash_b = 0
        self.flash_on = False

        self.start_can()

    def led_service_callback(self, request, response):
        self.get_logger().info(f"Received request: {request}")

        if request.flash:
            # Stop any ongoing color timers
            self.green_timer.cancel()
            self.blue_timer.cancel()

            # Stop previous flashing if active
            if self.flash_timer:
                self.flash_timer.cancel()
                self.flash_timer = None

            # Clear all current LED states except green
            self.set_duty_cycle(self.RED_CONTROL_ID, 0)
            time.sleep(0.05)
            self.set_duty_cycle(self.BLUE_CONTROL_ID, 0)
            time.sleep(0.05)

            # Save values and start flashing (only for green for now)
            self.flash_g = request.g

            self.flash_timer = self.create_timer(0.5, self.toggle_flash)
            self.get_logger().info("Started flashing green.")
            response.success = True

        else:
            # Stop flashing
            if self.flash_timer:
                self.flash_timer.cancel()
                self.flash_timer = None
                self.get_logger().info("Stopped flashing.")

            # Turn off all LEDs before applying new values, can try if it still blinks into another color first when
            # sending anything except just R,G or B

            # self.set_duty_cycle(self.RED_CONTROL_ID, 0)
            # self.set_duty_cycle(self.GREEN_CONTROL_ID, 0)
            # self.set_duty_cycle(self.BLUE_CONTROL_ID, 0)

            self.set_duty_cycle(self.RED_CONTROL_ID, request.r)
            self.last_green = request.g
            self.last_blue = request.b
            self.green_timer.reset()
            self.blue_timer.reset()

            response.success = True

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

    def set_duty_cycle(self, control_id, level):
        data = [level, 0x00]
        self.send_can_message(control_id, data)

    def set_green(self):
        self.green_timer.cancel()
        self.set_duty_cycle(self.GREEN_CONTROL_ID, self.last_green)

    def set_blue(self):
        self.blue_timer.cancel()
        self.set_duty_cycle(self.BLUE_CONTROL_ID, self.last_blue)

    def toggle_flash(self):
        if self.flash_on:
            # Turn off LEDs (only green for now)
            self.set_duty_cycle(self.GREEN_CONTROL_ID, 0)
        else:
            # Turn on to flash color
            self.set_duty_cycle(self.GREEN_CONTROL_ID, self.flash_g)
        self.flash_on = not self.flash_on


def main(args=None):
    rclpy.init(args=args)
    node = LedStrip()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()