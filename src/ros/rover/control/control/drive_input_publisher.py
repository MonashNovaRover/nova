#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from core.msg import DriveInput
from std_msgs.msg import Header
import math
import threading

class DriveInputPublisher(Node):
    def __init__(self):
        super().__init__('drive_input_publisher')
        self.publisher = self.create_publisher(DriveInput, '/drive_input', 50)
        self.inputs_thread = threading.Thread(target=self.publish_drive_input)
        self.inputs_thread.start()

    def publish_drive_input(self):
        while True:
            speed = float(input("Enter Speed: "))
            radius = float(input("Enter Radius: "))
            direction = int(input("Enter Direction: "))

            drive_input_msg = DriveInput()

            drive_input_msg.speed = speed
            drive_input_msg.radius = radius
            drive_input_msg.mode = bytes([0])
            drive_input_msg.direction = direction
            drive_input_msg.handbrake = False

            self.publisher.publish(drive_input_msg)
            self.get_logger().info('Publishing DriveInput message')

def main(args=None):
    rclpy.init(args=args)
    drive_input_publisher = DriveInputPublisher()
    rclpy.spin(drive_input_publisher)
    drive_input_publisher.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
