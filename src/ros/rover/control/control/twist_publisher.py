#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from std_msgs.msg import Header
import math
import time
import threading

class TwistPublisher(Node):
    def __init__(self):
        super().__init__('twist_publisher')
        self.twist_msg = Twist()
        self.publisher = self.create_publisher(Twist, '/cmd_vel', 50)
        self.inputs_thread = threading.Thread(target=self.publish_twist)
        self.inputs_thread.start()
        while True:
            linear = float(input("Enter Linear: "))
            angular = float(input("Enter Angular: "))

            self.twist_msg.linear.x = linear
            self.twist_msg.angular.z = angular


    def publish_twist(self):
        while True:
            self.publisher.publish(self.twist_msg)
            time.sleep(0.01)

def main(args=None):
    rclpy.init(args=args)
    twist_publisher = TwistPublisher()
    rclpy.spin(twist_publisher)
    twist_publisher.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
