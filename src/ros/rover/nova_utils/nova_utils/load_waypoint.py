#!/usr/bin/env python3
"""
Node that loads waypoints from a file and sends it to the /navigate_through_poses action
server. It continuously checks the status of the action server to monitor if it has been
aborted. If the action server has aborted, it will reload the waypoints to restart navigation.

Authors: Tarik Thomas, Terry Tian
"""

import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from rclpy.qos import QoSProfile, QoSDurabilityPolicy
from geometry_msgs.msg import PoseStamped
from nav2_msgs.action import NavigateThroughPoses
from action_msgs.msg import GoalStatus
from visualization_msgs.msg import Marker, MarkerArray
import json
import os
import sys
import time

class WaypointNavigator(Node):
    def __init__(self, file_path):
        super().__init__('waypoint_navigator')

        # ✅ Action client for Nav2 NavigateThroughPoses
        self._action_client = ActionClient(self, NavigateThroughPoses, '/navigate_through_poses')

        # ✅ Load waypoints from JSON
        self._file_path = file_path if file_path else os.path.expanduser("~/nova/src/ros/rover/auto_bringup/params/waypoints.json")
        self._waypoints = self.load_waypoints()

        if not self._waypoints:
            self.get_logger().error("❌ No waypoints found or failed to load JSON. Exiting.")
            return

        # ✅ Wait for the Nav2 action server
        self.get_logger().info("⏳ Waiting for /navigate_through_poses action server...")
        if not self._action_client.wait_for_server(timeout_sec=10.0):
            self.get_logger().error("❌ Action server not available. Exiting.")
            return
        self.get_logger().info("✅ Action server available!")

        # ✅ Send waypoints asynchronously as an action goal
        self.send_goal_async()

        # Create a timer to check navigation status every second
        self._nav_check_timer = self.create_timer(1.0, self.check_nav_status)


    def load_waypoints(self):
        """Loads waypoints from JSON file and converts them into PoseStamped messages."""
        if not os.path.exists(self._file_path):
            self.get_logger().error(f"❌ Waypoints file not found: {self._file_path}")
            return None

        with open(self._file_path, 'r') as f:
            data = json.load(f)

        waypoints_data = data.get("waypoints", [])
        if not waypoints_data:
            self.get_logger().warn("⚠️ No waypoints found in the JSON file.")
            return None

        waypoints = []
        for idx, wp in enumerate(waypoints_data):
            pose = PoseStamped()
            pose.header.frame_id = "map"
            pose.header.stamp = self.get_clock().now().to_msg()
            pose.pose.position.x = wp["position"]["x"]
            pose.pose.position.y = wp["position"]["y"]
            pose.pose.position.z = wp["position"]["z"]
            pose.pose.orientation.x = wp["orientation"]["x"]
            pose.pose.orientation.y = wp["orientation"]["y"]
            pose.pose.orientation.z = wp["orientation"]["z"]
            pose.pose.orientation.w = wp["orientation"]["w"]
            waypoints.append(pose)
            self.get_logger().info(f"📍 Loaded Waypoint {idx+1}: ({pose.pose.position.x}, {pose.pose.position.y})")

        return waypoints


    def publish_waypoint_markers(self):
        """Publishes waypoints as markers to RViz for visualization. Currently not used."""
        marker_array = MarkerArray()

        for idx, pose in enumerate(self._waypoints):
            marker = Marker()
            marker.header.frame_id = "map"
            marker.header.stamp = self.get_clock().now().to_msg()
            marker.ns = "waypoints"
            marker.id = idx
            marker.type = Marker.ARROW  # arrow to indicate waypoints
            marker.action = Marker.ADD
            marker.pose = pose.pose  # Use the same pose as the waypoint
            marker.scale.x = 0.15  # Size of marker
            marker.scale.y = 0.15
            marker.scale.z = 0.15
            marker.color.r = 0.0  # Green
            marker.color.g = 1.0
            marker.color.b = 0.0
            marker.color.a = 1.0  # Fully visible

            text_marker = Marker()
            text_marker.header.frame_id = "map"
            text_marker.header.stamp = self.get_clock().now().to_msg()
            text_marker.ns = "waypoints_labels"
            text_marker.id = idx + 1000  # Offset to avoid ID conflict
            text_marker.type = Marker.TEXT_VIEW_FACING
            text_marker.action = Marker.ADD
            text_marker.pose = pose.pose
            text_marker.pose.position.z += 0.3  # Raise text above the marker
            text_marker.scale.z = 0.2  # Text size
            text_marker.color.r = 1.0  # White text
            text_marker.color.g = 1.0
            text_marker.color.b = 1.0
            text_marker.color.a = 1.0
            text_marker.text = f"wp_{idx+1}"  # Label as "wp_1", "wp_2", etc.

            marker_array.markers.append(marker)
            marker_array.markers.append(text_marker)

        self.marker_pub.publish(marker_array)
        self.get_logger().info(f"📌 Published {len(self._waypoints)} waypoints to /waypoints for visualization.")


    def send_goal_async(self):
        """Sends the waypoints to the NavigateThroughPoses action server."""
        goal_msg = NavigateThroughPoses.Goal()
        goal_msg.poses = self._waypoints  # List of PoseStamped

        # ✅ Send goal and wait for acceptance
        # for pose_stamped in goal_msg.poses:
        #     self.get_logger().warn(f"{pose_stamped.pose.position.z}")
            
        self.get_logger().info("🚀 Sending waypoints to /navigate_through_poses...")
        send_future = self._action_client.send_goal_async(goal_msg)
        send_future.add_done_callback(self.goal_response_callback)


    def goal_response_callback(self, future):
        goal_handle = future.result()
        if not goal_handle.accepted:
            self.get_logger().error("❌ Goal was rejected by the action server.")
            return

        self.get_logger().info("✅ Goal accepted. Navigating through waypoints...")
        self._goal_handle = goal_handle

        # Set up asynchronous result callback
        result_future = goal_handle.get_result_async()
        result_future.add_done_callback(self.result_callback)

    
    def result_callback(self, future):
        result = future.result()
        if result.status == GoalStatus.STATUS_SUCCEEDED:
            self.get_logger().info("✅ Navigation through all waypoints succeeded!")
        elif result.status == GoalStatus.STATUS_CANCELED:
            self.get_logger().warn("⚠️ Navigation was canceled before completion.")
        elif result.status == GoalStatus.STATUS_ABORTED:
            self.get_logger().error("❌ Navigation failed.")
        else:
            self.get_logger().error(f"❓ Navigation ended with unknown status: {result.status}")


    def check_nav_status(self):
        status = self._goal_handle.status
        if status == GoalStatus.STATUS_ABORTED:
            self.get_logger().info("❌ Navigation aborted detected by timer callback!")
            self.get_logger().info("🚀 Sending waypoints to restart navigation")
            self._waypoints = self.load_waypoints()
            self.send_goal_async()


def main(args=None):
    """Main function to start the ROS2 node."""
    rclpy.init(args=args)

    # ✅ Allow optional file path argument
    file_path = sys.argv[1] if len(sys.argv) > 1 else None
    node = WaypointNavigator(file_path)

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()