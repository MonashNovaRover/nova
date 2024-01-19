#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from core.msg import DriveInputStamped
from std_msgs.msg import Header
import math
import threading

class DriveInputStampedPublisher(Node):
    def __init__(self):
        super().__init__('drive_input_stamped_publisher')
        self.publisher = self.create_publisher(DriveInputStamped, '/pivot_drive_controller/cmd_vel', 50)
        self.inputs_thread = threading.Thread(target=self.publish_drive_input_stamped)
        self.inputs_thread.start()

    def publish_drive_input_stamped(self):
        while True:
            speed = float(input("Enter Speed: "))
            radius = float(input("Enter Radius: "))
            direction = int(input("Enter Direction: "))
            drive_input_stamped_msg = DriveInputStamped()
            drive_input_stamped_msg.header = Header()
            drive_input_stamped_msg.header.stamp = self.get_clock().now().to_msg()
            drive_input_stamped_msg.header.frame_id = 'base_link'
            drive_input_stamped_msg.drive_input.speed = speed
            drive_input_stamped_msg.drive_input.radius = radius
            drive_input_stamped_msg.drive_input.mode = bytes([0])
            drive_input_stamped_msg.drive_input.direction = direction
            drive_input_stamped_msg.drive_input.handbrake = False

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
