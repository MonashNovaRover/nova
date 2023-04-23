#!/usr/bin/env python3
__package__ = "autonomous"

import rclpy
from rclpy.node import Node
from core.msg import AutonomousGoal, AutonomousGoalArray, Point2D
from autonomous.config.ros_config import auto_goal_topic


class GoalPublisher(Node):
    def __init__(self):
        super().__init__("goal_publisher")
        self.publisher = self.create_publisher(AutonomousGoalArray, auto_goal_topic, 10)
        self.timer = self.create_timer(0.1, self.get_input)

    def get_input(self):
        """
        Take terminal inputs to publish a new goal, including any intermediate goals required on the way to the 
        goal. The goal is published as an AutonomousGoalArray, with the main goal as the last element in the array.
        Assumes latitude, longitude coordinates in decimal form (ie 40.443, -79.944)
        """
        # take user input for main goal
        coord = input("Enter new goal as lat, lon tuple: ").replace(",", " ")
        ids_string = input("Enter integer ids of AR beacons if any: ").replace(",", " ")

        # Make AutonomousGoal out of user input
        lat, lon = float(coord.split()[0]), float(coord.split()[1])
        ids = [int(id_string) for id_string in ids_string.split()]

        main_goal = AutonomousGoal()
        position = Point2D()

        position.x, position.y = lat, lon
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
        goals.header.frame_id = "world"
        goals.header.stamp = self.get_clock().now().to_msg()

        intermediates = int(input("Enter number of intermediate goals (0 for none): "))

        # Get intermediate goals
        for i in range(intermediates):
            coord = input(f"Enter intermediate goal {i + 1} as lat, lon tuple: ")
            lat, lon = float(coord.split()[0]), float(coord.replace(",", " ").split()[1])
            intermediate_goal = AutonomousGoal()
            intermediate_goal.type = AutonomousGoal.GOAL_TYPE_INTERMEDIATE
            intermediate_position = Point2D()
            intermediate_position.x, intermediate_position.y = lat, lon
            intermediate_goal.position = intermediate_position
            goals.goals.append(intermediate_goal)

        goals.goals.append(main_goal)

        self.get_logger().info(f"Publishing new goals: {goals}")

        self.publisher.publish(goals)


def main(args=None):
    rclpy.init(args=args)
    pub = GoalPublisher()
    rclpy.spin(pub)
    pub.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
