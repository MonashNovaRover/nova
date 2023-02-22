#!/usr/bin/env python3
import serial

import rclpy
from rclpy.node import Node

from core.msg import RTCM3.msg

class SubToBaseNode(Node):
    def __init__(self):
        super().__init__('getBeseCorrection_pub')
        self.subscription = self.create_subscription(
            UBXNavHPPosLLH,
            'ubx_nav_hp_pos_llh',
            self.callback_func,
            10)
        self.subscription  # prevent unused variable warning

    def callback_func(self, msg):
        self.get_logger().info("Received: '%s'" % msg.data)

def main (args = None):
    rclpy.init(args = args)
    subscriber = SubToBaseNode()
    rclpy.spin(subscriber)

    subscriber.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()