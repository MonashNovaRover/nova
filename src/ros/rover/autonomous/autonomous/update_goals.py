#!/usr/bin/env python3
__package__ = "autonomous"

import rclpy
from rclpy.node import Node
from std_msgs.msg import Empty
from core.msg import AutonomousGoal, AutonomousGoalArray, Point2D
from autonomous.config.ros_config import auto_goal_topic, auto_goal_gps


class GoalPublisher(Node):
    def __init__(self):
        super().__init__("goal_publisher")
        self.param_do_gps = self.declare_parameter("do_gps", True).value
        if self.param_do_gps:
            # Publish goals to pose_converter to turn into x, y coords
            goal_topic = auto_goal_gps
        else:
            # Publish direct x, y coords
            goal_topic = auto_goal_topic
        self.pub_goals = self.create_publisher(AutonomousGoalArray, goal_topic, 10)
        self.pub_return = self.create_publisher(Empty, "/autonomous/return", 10)
        self.timer = self.create_timer(0.1, self.get_input)

    def get_input(self):
        """
        Take terminal inputs to publish a new goal, including any intermediate goals required on the way to the 
        goal. The goal is published as an AutonomousGoalArray, with the main goal as the last element in the array.
        Assumes latitude, longitude coordinates in decimal form (ie 40.443, -79.944)
        """
        # take user input for main goal
        if self.param_do_gps:
            coord = input("Enter new goal as lat, lon tuple, or type 'return': ").replace(",", " ").strip()
        else:
            coord = input("Enter new goal as x, y tuple, or type 'return': ").replace(",", " ")
        if coord[0].lower() == 'r':
            self.pub_return.publish(Empty())
            return
        ids_string = input("Enter integer ids of AR beacons if any: ").replace(",", " ").strip()

        # Make AutonomousGoal out of user input
        x, y = float(coord.split()[0]), float(coord.split()[1])
        ids = [int(id_string) for id_string in ids_string.split()]

        main_goal = AutonomousGoal()
        position = Point2D()

        position.x, position.y = x, y
        # Check the number of AR tags to work out what type of goal we're going to
        main_goal.tag_ids = ids
        if len(ids) == 0:
            main_goal.type = AutonomousGoal.GOAL_TYPE_COORDINATE
        elif len(ids) == 1:
            main_goal.type = AutonomousGoal.GOAL_TYPE_TAG
        elif len(ids) == 2:
            main_goal.type = AutonomousGoal.GOAL_TYPE_GATE
        else:
            self.get_logger().error("Invalid number of AR tag ids")
            return
        main_goal.position = position

        # Array to store intermediate and main goals
        goals = AutonomousGoalArray()
        goals.header.frame_id = "map"
        goals.header.stamp = self.get_clock().now().to_msg()

        intermediates = int(input("Enter number of intermediate goals (0 for none): "))

        # Get intermediate goals
        for i in range(intermediates):
            coord = input(f"Enter intermediate goal {i + 1} as lat, lon tuple: ").replace(",", " ").strip()
            lat, lon = float(coord.split()[0]), float(coord.split()[1])
            intermediate_goal = AutonomousGoal()
            intermediate_goal.type = AutonomousGoal.GOAL_TYPE_INTERMEDIATE
            intermediate_position = Point2D()
            intermediate_position.x, intermediate_position.y = lat, lon
            intermediate_goal.position = intermediate_position
            goals.goals.append(intermediate_goal)

        goals.goals.append(main_goal)

        self.get_logger().info(f"Publishing new goals: {goals}")

        self.pub_goals.publish(goals)


def main(args=None):
    rclpy.init(args=args)
    pub = GoalPublisher()
    rclpy.spin(pub)
    pub.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
