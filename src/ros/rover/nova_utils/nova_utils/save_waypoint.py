#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import json
import os
import re


class WaypointRecorder(Node):
    def __init__(self):
        super().__init__('waypoint_recorder')
        self.subscription = self.create_subscription(
            String, '/blackboard', self.blackboard_callback, 1)
        self.get_logger().info("🚀 WaypointRecorder started! Listening to /blackboard...")
        self.file_path = os.path.join(os.path.expanduser("~"), "waypoints.json")

    def blackboard_callback(self, msg):
        """ Extracts waypoints from the 'goals' section of the blackboard topic and saves them."""
        waypoints = []
        try:
            string_goals = msg.data.split('goals: ')[1].split('\n')[0].split('(')[1:]
            for string_goal in string_goals:
                coords = string_goal.split(')')[0].split(', ')
                pos_x = float(coords[0])
                pos_y = float(coords[1])
                pos_z = float(coords[2])
                ori_x = float(coords[3])
                ori_y = float(coords[4])
                ori_z = float(coords[5])
                ori_w = float(coords[6])
                waypoints.append({
                    "position": {"x": pos_x, "y": pos_y, "z": pos_z},
                    "orientation": {"x": ori_x, "y": ori_y, "z": ori_z, "w": ori_w}
                })
            
        except Exception as e:
            self.get_logger().warn(f'Error in extracting waypoints: {e}')
            return None

        if waypoints:
            self.save_waypoints(waypoints)


    def save_waypoints(self, waypoints):
        """ Saves the extracted waypoints to a JSON file. """
        with open(self.file_path, 'w') as f:
            json.dump({"waypoints": waypoints}, f, indent=2)
        print(f"📁 Waypoints saved to: {self.file_path}")
        self.get_logger().info(f"Waypoints saved to: {self.file_path}")


def main(args=None):
   rclpy.init(args=args)
   node = WaypointRecorder()
   rclpy.spin(node) # Node will exit automatically after capturing the goals


if __name__ == '__main__':
   main()