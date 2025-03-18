#!/usr/bin/env python3

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
        self.start_can()

    def led_service_callback(self, request, response):
        self.get_logger().info(f"Received service request: {request}")
        self.set_duty_cycle(self.RED_CONTROL_ID, request.r)
        self.set_duty_cycle(self.GREEN_CONTROL_ID, request.g)
        self.set_duty_cycle(self.BLUE_CONTROL_ID, request.b)
        response.success = True
        return response

    def send_can_message(self, frame_id, data):
        frame = jcan.Frame(frame_id, data)
        try:
            self.get_logger().info(f"Sending {frame}")
            self.bus.send(frame)
        except Exception as e:
            self.get_logger().error(f"Failed to send CAN message: {e}")

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