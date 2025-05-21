#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gz_imu_fixer
TOPICS:
  - subscriber: /gz/oak/imu/transformed [Imu]
  - publisher: /oak/imu/transformed     [Imu]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_utils
EDITED BY:	Victor Bartlinski
CREATION:	27/04/2025
EDITED:		30/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile

from sensor_msgs.msg import Imu

class GzImuFixer(Node):

    def __init__(self):
        super().__init__('gz_imu_fixer')
        self.sub_topic = self.declare_parameter('sub_topic', '/gz/camera/imu/transformed').value
        self.pub_topic = self.declare_parameter('pub_topic', '/camera/imu/transformed').value
        self.sub_imu = self.create_subscription(Imu, self.sub_topic, self.sub_callback, 10)
        self.pub_imu = self.create_publisher(Imu, self.pub_topic, 10)

    def sub_callback(self, msg):
        msg.angular_velocity.z = 0.0
        msg.linear_acceleration.z = 0.0
        self.pub_imu.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = GzImuFixer()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()

