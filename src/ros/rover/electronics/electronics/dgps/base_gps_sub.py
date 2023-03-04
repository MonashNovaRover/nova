#!/usr/bin/env python3
import serial

import rclpy
from rclpy.node import Node

from core.msg import RTCM3

class SubToBaseNode(Node):
    def __init__(self, com_no, baud):
        super().__init__('getBeseCorrection_pub')

        self.ser = serial.Serial()
        self.config_port(com_no, baud)

        self.subscription = self.create_subscription(
            RTCM3,
            'gps_base/rtcm_out', 
            self.callback_func,
            10)
        self.subscription


    def callback_func(self, msg):
        self.get_logger().info("Received: '%d'" % msg.data)

        self.ser.write(msg.data)

    def config_port(self, port_name, baud):
        self.ser.baudrate = baud
        if port_name == "":
            port_name = "/dev/ttyUSB0"
        self.ser.port = port_name
        self.ser.open()

def main (args = None):
    rclpy.init(args = args)
    subscriber = SubToBaseNode()
    rclpy.spin(subscriber)

    subscriber.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()