#!/usr/bin/env python3
import serial

import rclpy
from rclpy.node import Node

from ublox_ubx_msgs.msg import RTCM3
from rclpy.logging import LoggingSeverity

from rclpy.qos import qos_profile_sensor_data as qos

class SubToBaseNode(Node):
    def __init__(self, com_no, baud):
        super().__init__('getBaseCorrection_pub')

        self.ser = serial.Serial()
        self.config_port(com_no, baud)

        self.count = 0

        self.subscription = self.create_subscription(
            RTCM3,
            'gps_base/rtcm3_out', 
            self.callback_func,
            qos)


    def callback_func(self, msg):
        self.get_logger().info(f"Received: {msg.data}")

        # if self.count > 50:                 # need to test w/out, error may have been fixed by different section
        #     self.count = 0
        #     self.ser.reset_output_buffer()

        self.ser.write(msg.data)
        self.count += 1

    def config_port(self, port_name, baud):
        self.ser.baudrate = baud
        if port_name == "":
            port_name = "/dev/ttyUSB0"
        self.ser.port = port_name
        self.ser.open()

    def parse_rtcm_out(self, rtcm_msg):
        msg_str = f"raw rtcm: {rtcm_msg}"
        self.get_logger().debug(msg_str,LoggingSeverity.INFO,throttle_duration_sec=2)

def main (args = None):
    rclpy.init(args = args)
    subscriber = SubToBaseNode("", 115200)
    rclpy.spin(subscriber)

    subscriber.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()

