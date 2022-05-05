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
        ids_string = input("Enter integer ids of AR beacons if any: ")
        x, y = float(coord.split()[0]), float(coord.split()[1])
        ids = [int(id_string) for id_string in ids_string.split()]

        self.get_logger().info(f"Publishing new goal: x = {x}, y = {y}, ids = ({ids_string})")

        info = AutonomousInfo()
        position = Point2D()

        position.x, position.y = x, y
        info.ids = ids
        info.position = position

        self.publisher.publish(info)

def main(args = None):
    rclpy.init(args=args)
    pub = GoalPublisher()
    rclpy.spin(pub)
    pub.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
