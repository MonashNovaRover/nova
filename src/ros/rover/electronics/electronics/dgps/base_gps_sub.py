#!/usr/bin/env python3
import serial

import rclpy
from rclpy.node import Node

from core.msg import RTCM3

class SubToBaseNode(Node):
    def __init__(self):
        super().__init__('getBeseCorrection_pub')
        self.subscription = self.create_subscription(
            RTCM3,
            'gps_base/rtcm_out', 
            self.callback_func,
            10)
        self.subscription  # prevent unused variable warning
        # remember to add node name when known

    def callback_func(self, msg):
        self.get_logger().info("Received: '%d'" % msg.data)

    

    def parse_rtcm(self, data):
        dd

def main (args = None):
    rclpy.init(args = args)
    subscriber = SubToBaseNode()
    rclpy.spin(subscriber)

    subscriber.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()