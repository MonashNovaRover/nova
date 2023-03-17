#!/usr/bin/env python3
import serial

import rclpy
from rclpy.node import Node

from ublox_ubx_msgs.msg import RTCM3
from rclpy.logging import LoggingSeverity

# from rclpy.qos import QoSDurabilityPolicy, QoSHistoryPolicy, QoSReliabilityPolicy
# from rclpy.qos import QoSProfile

from rclpy.qos import qos_profile_sensor_data as qos

# def generate_qos_profile():
#     profile = QoSProfile()
    
#     # RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT => UDP (won't keep history)
#     # RMW_QOS_POLICY_RELIABILITY_RELIABLE    => TCP (keeps history) 
#     profile.reliability = QoSReliabilityPolicy.RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT
#     from rclpy.qos_event import QoSOfferedDeadlineMissedInfo

#     return profile

class SubToBaseNode(Node):
    def __init__(self, com_no, baud):
        super().__init__('getBaseCorrection_pub')

        # self.qos = generate_qos_profile()

        self.ser = serial.Serial()
        self.config_port(com_no, baud)

        self.count = 0

        self.get_logger().info("hype")

        self.subscription = self.create_subscription(
            RTCM3,
            'gps_base/rtcm3_out', 
            self.callback_func,
            qos)


    def callback_func(self, msg):
        self.get_logger().info(f"Received: {msg.data}")

        if self.count > 50:
            self.count = 0
            self.ser.reset_output_buffer()

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
        self.get_logger().log(msg_str,LoggingSeverity.INFO,throttle_duration_sec=2)

def main (args = None):
    rclpy.init(args = args)
    subscriber = SubToBaseNode("", 115200)
    rclpy.spin(subscriber)

    subscriber.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()

