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
           String, '/blackboard', self.blackboard_callback, 10)
       self.get_logger().info("🚀 WaypointRecorder started! Listening to /blackboard...")
       self.file_path = os.path.join(os.path.expanduser("~"), "waypoints.json")


   def blackboard_callback(self, msg):
       """ Extracts waypoints from the 'goals' section of the blackboard topic and saves them. """
       pattern = r"Position: \(([-0-9.]+), ([-0-9.]+), ([-0-9.]+)\) Orientation: \(([-0-9.]+), ([-0-9.]+), ([-0-9.]+), ([-0-9.]+)\)"


       waypoints = []
       for match in re.finditer(pattern, msg.data):
           pos_x, pos_y, pos_z, ori_x, ori_y, ori_z, ori_w = map(float, match.groups())


           waypoints.append({
               "position": {"x": pos_x, "y": pos_y, "z": pos_z},
               "orientation": {"x": ori_x, "y": ori_y, "z": ori_z, "w": ori_w}
           })


       if waypoints:
           # Save the waypoints and shutdown since we only need this data once
           self.save_waypoints(waypoints)
           rclpy.shutdown()


   def save_waypoints(self, waypoints):
       """ Saves the extracted waypoints to a JSON file. """
       with open(self.file_path, 'w') as f:
           json.dump({"waypoints": waypoints}, f, indent=2)
       print(f"📁 Waypoints saved to: {self.file_path}")
       self.get_logger().info(f"Waypoints saved to: {self.file_path}")


def main(args=None):
   rclpy.init(args=args)
   node = WaypointRecorder()
   rclpy.spin(node)  # Node will exit automatically after capturing the goals


if __name__ == '__main__':
   main()



