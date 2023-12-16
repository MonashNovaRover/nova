#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from core.msg import DriveInputStamped
from std_msgs.msg import Header
import math

class DriveInputStampedPublisher(Node):
    def __init__(self):
        super().__init__('drive_input_stamped_publisher')
        self.publisher = self.create_publisher(DriveInputStamped, '/pivot_drive_controller/drive_input_cmd', 50)
        self.timer = self.create_timer(0.02, self.publish_drive_input_stamped)

    def publish_drive_input_stamped(self):
        drive_input_stamped_msg = DriveInputStamped()
        drive_input_stamped_msg.header = Header()
        drive_input_stamped_msg.header.stamp = self.get_clock().now().to_msg()
        drive_input_stamped_msg.header.frame_id = 'base_link'
        drive_input_stamped_msg.speed = 1.0
        drive_input_stamped_msg.radius = math.inf
        drive_input_stamped_msg.mode = bytes([0])
        drive_input_stamped_msg.direction = -1
        drive_input_stamped_msg.handbrake = False

        self.publisher.publish(drive_input_stamped_msg)
        self.get_logger().info('Publishing DriveInputStamped message')

def main(args=None):
    rclpy.init(args=args)
    drive_input_stamped_publisher = DriveInputStampedPublisher()
    rclpy.spin(drive_input_stamped_publisher)
    drive_input_stamped_publisher.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
