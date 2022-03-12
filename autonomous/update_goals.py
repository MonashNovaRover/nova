#!/usr/bin/env python3
__package__ = "autonomous"

import rclpy
from rclpy.node import Node
from core.msg import AutonomousGoal, Point2D
from config.ros_config import auto_goals_topic

class GoalPublisher(Node):
    def __init__(self):
        super().__init__("goal_publisher")
        self.publisher = self.create_publisher(AutonomousGoal, auto_goals_topic, 10)
        self.timer = self.create_timer(0.1, self.update_goal)

    def update_goal(self):
        coord = input("Enter new goal as tuple: ")
        iD = int(input("Enter integer id of AR beacon: "))
        x, y = float(coord.split()[0]), float(coord.split()[1])

        goal = AutonomousGoal()
        position = Point2D()

        position.x, position.y = x, y
        goal.id = iD
        goal.position = position

        self.publisher.publish(goal)
        self.get_logger().info(f"Publishing new goal: x = {x}, y = {y}, id = {iD}")

def main(args = None):
    rclpy.init(args=args)
    pub = GoalPublisher()
    rclpy.spin(pub)
    pub.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
