#!/usr/bin/env python3
import serial

import rclpy
from rclpy.node import Node

from core.msg import RTCM3
from rclpy.logging import LoggingSeverity

class SubToBaseNode(Node):
    def __init__(self, com_no, baud):
        super().__init__('getBaseCorrection_pub')

        self.ser = serial.Serial()
        self.config_port(com_no, baud)

        self.get_logger().info("hype")

        self.subscription = self.create_subscription(
            RTCM3,
            '/gps_base/rtcm3_out', 
            self.callback_func,
            10)


    def callback_func(self, msg):
        self.get_logger().info("Received: '%d'" % msg.data)

        self.ser.write(msg.data)

    def config_port(self, port_name, baud):
        self.ser.baudrate = baud
        if port_name == "":
            port_name = "/dev/ttyUSB0"
        self.ser.port = port_name
        self.ser.open()

    def parse_rtcm_out(self, rtcm_msg):
        msg_str = f"raw rtcm: {rtcm_msg}"
        self.get_logger().log(msg_str,LoggingSeverity.INFO,throttle_duration_sec=2)

def main (args = None):
    rclpy.init(args = args)
    subscriber = SubToBaseNode("", 115200)
    rclpy.spin(subscriber)

    subscriber.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()

