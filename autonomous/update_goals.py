#!/usr/bin/env python3
__package__ = "autonomous"

import rclpy
from rclpy.node import Node
from core.msg import AutonomousInfo, Point2D
from config.ros_config import auto_goals_info


class GoalPublisher(Node):
    def __init__(self):
        super().__init__("goal_publisher")
        self.publisher = self.create_publisher(AutonomousInfo, auto_goals_info, 10)
        self.timer = self.create_timer(0.1, self.get_input)

    def get_input(self):
        coord = input("Enter new goal as tuple: ")
        id = int(input("Enter integer id of AR beacon: "))
        x, y = float(coord.split()[0]), float(coord.split()[1])

        ar = input("Does the goal have an AR tag? (y/n)")[0].lower()
        is_ar = ar == "y"
        gate = input("Does the goal have a gate? (y/n)")[0].lower()
        is_gate = gate == "y"

        self.get_logger().info(f"Publishing new goal: x = {x}, y = {y}, id = {id}")

        info = AutonomousInfo()
        position = Point2D()

        position.x, position.y = x, y
        info.id = id
        info.position = position

        info.is_ar_tag = is_ar
        info.is_gate = is_gate

        self.publisher.publish(info)

def main(args = None):
    rclpy.init(args=args)
    pub = GoalPublisher()
    rclpy.spin(pub)
    pub.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
