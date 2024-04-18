#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from core.msg import DriveInput, DriveInputStamped
from std_msgs.msg import Header
import math
import threading

class DriveInputStampedPublisher(Node):
    def __init__(self):
        super().__init__('drive_input_stamped_publisher')
        self.publisher = self.create_publisher(DriveInputStamped, '/pivot_drive_controller/drive_input', 50)
        self.inputs_thread = threading.Thread(target=self.publish_drive_input_stamped)
        self.inputs_thread.start()
        self.current_msg = self.create_drive_input_msg(0.0, float('inf'), 0)

        self.create_timer(0.02, self.publish_msg)

    def publish_drive_input_stamped(self):
        while True:
            speed = float(input("Enter Speed: "))
            radius = float(input("Enter Radius: "))
            direction = int(input("Enter Direction: "))
            self.current_msg = self.create_drive_input_msg(speed, radius, direction)


    def create_drive_input_msg(self, speed, radius, direction):
        drive_input_msg = DriveInput()
        drive_input_msg.speed = speed
        drive_input_msg.radius = radius
        drive_input_msg.mode = bytes([0])
        drive_input_msg.direction = direction
        drive_input_msg.handbrake = False

        return drive_input_msg

    def publish_msg(self):
        drive_input_stamped_msg = DriveInputStamped()
        drive_input_stamped_msg.header = Header()
        drive_input_stamped_msg.header.stamp = self.get_clock().now().to_msg()
        drive_input_stamped_msg.header.frame_id = 'base_link'

        drive_input_stamped_msg.drive_input = self.current_msg

        self.publisher.publish(drive_input_stamped_msg)


def main(args=None):
    rclpy.init(args=args)
    drive_input_stamped_publisher = DriveInputStampedPublisher()
    rclpy.spin(drive_input_stamped_publisher)
    drive_input_stamped_publisher.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
