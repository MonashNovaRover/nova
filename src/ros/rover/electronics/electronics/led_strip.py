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
AUTHOR(S):      Matthew Hee
CREATION:       18/03/2025
EDITED:         18/03/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy, jcan
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

        self.green_timer = self.create_timer(0.1, self.set_green, autostart=False)
        self.blue_timer = self.create_timer(0.2, self.set_blue, autostart=False)

        self.last_green = 0
        self.last_blue = 0

        self.start_can()

    def led_service_callback(self, request, response):
        self.get_logger().info(f"Received service request: {request}")
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

    def set_green(self):
        self.green_timer.cancel()
        self.set_duty_cycle(self.GREEN_CONTROL_ID, self.last_green)

    def set_blue(self):
        self.blue_timer.cancel()
        self.set_duty_cycle(self.BLUE_CONTROL_ID, self.last_blue)

    def set_duty_cycle(self, control_id, level):
        data = [level, 0x00]
        self.send_can_message(control_id, data)

def main(args=None):
    rclpy.init(args=args)
    node = LedStrip()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()