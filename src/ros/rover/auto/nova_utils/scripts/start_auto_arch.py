#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Node that loads waypoints from a file and 
sends it to the /navigate_through_poses action
server. It continuously checks the status of the 
action server to monitor if it has been
aborted. If the action server has aborted, it will 
reload the waypoints to restart navigation.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: WaypointNavigator
TOPICS:
    - subscriber: /blackboard       [std_msgs/msg/String]
    - publisher: /auto/status       [nova_interfaces/msg/Status]
ACTIONS: 
  - client: /navigate_through_poses [NavigateThroughPosesNavigator]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	nova_utils
AUTHOR(S):	Tarik Thomas, Terry Tian, 
            Victor Bartlinski
CREATION:	24/03/2026
EDITED:		24/03/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from rclpy.client import Client
from rclpy.qos import QoSProfile, QoSDurabilityPolicy, QoSPresetProfiles
from rclpy.executors import MultiThreadedExecutor
from std_msgs.msg import String
from geometry_msgs.msg import PoseStamped, Point
from nav2_msgs.action import NavigateThroughPoses
from action_msgs.msg import GoalStatus
from visualization_msgs.msg import Marker, MarkerArray
import json
import os
import time


class WaypointNavigator(Node):
    def __init__(self):
        super().__init__('waypoint_navigator')

        self.file_path = self.declare_parameter(
            name='file_path', 
            value=os.path.expanduser('~/nova/src/ros/rover/auto/auto_bringup/params/waypoints.json'), 
        ).value
        self.waypoints = self.load_json_waypoints()
        self.blackboard = dict() # Dictionary to store blackboard data
        self.goal_handle = None

        # Create action client for /navigate_through_poses
        self.action_client = ActionClient(self, NavigateThroughPoses, '/navigate_through_poses')
        self.get_logger().info('⏳ Waiting for NavigateThroughPoses action server...')
        if not self.action_client.wait_for_server(timeout_sec=10.0):
            self.get_logger().error('❌ Action service /navigate_through_poses not available. Exiting.')
            return
        self.get_logger().info('✅ Action service /navigate_through_poses available!')

        # Save waypoints
        self.sub_blackboard = self.create_subscription(String, '/blackboard', self.blackboard_callback, QoSPresetProfiles.SENSOR_DATA.value)
        self.get_logger().info('✅ Subscriber /blackboard created!')

        # Start navigation
        self.start_navigation()

    def start_navigation(self):
        '''Starts the navigation by sending the waypoints to the action server.'''
        # Send waypoints asynchronously as an action goal
        self.send_goal_async()

        # Create a timer to check navigation status every second
        self.nav_check_timer = self.create_timer(1.0, self.check_nav_status)

    def check_nav_status(self):
        if not self.goal_handle:
            self.get_logger().warn('⚠️ No active goal handle. Skipping navigation status check.')
            return

        if self.goal_handle.status == GoalStatus.STATUS_ABORTED:
            self.get_logger().info('❌ Navigation aborted detected by timer callback!')
            self.get_logger().info('🚀 Sending waypoints to restart navigation')
            self.waypoints = self.load_json_waypoints()
            self.send_goal_async()

    def send_goal_async(self):
        '''Sends the waypoints asynchronously to the NavigateThroughPoses action server.'''
        goal_msg = NavigateThroughPoses.Goal()
        goal_msg.poses = self.waypoints  # List of PoseStamped
        self.get_logger().info('🚀 Sending waypoints to /navigate_through_poses...')
        send_future = self.action_client.send_goal_async(goal_msg)
        send_future.add_done_callback(self.response_goal_callback)

    def response_goal_callback(self, future):
        goal_handle = future.result()
        if not goal_handle.accepted:
            self.get_logger().error('❌ Goal was rejected by the action server.')
            return

        self.get_logger().info('✅ Goal accepted. Navigating through waypoints...')
        self.goal_handle = goal_handle

        # Set up asynchronous result callback
        result_future = goal_handle.get_result_async()
        result_future.add_done_callback(self.result_goal_callback)

    def result_goal_callback(self, future):
        result = future.result()
        if result.status == GoalStatus.STATUS_SUCCEEDED:
            self.get_logger().info('✅ Navigation through all waypoints succeeded!')
        elif result.status == GoalStatus.STATUS_CANCELED:
            self.get_logger().warn('⚠️ Navigation was canceled before completion.')
        elif result.status == GoalStatus.STATUS_ABORTED:
            self.get_logger().error('❌ Navigation failed.')
        else:
            self.get_logger().error(f'❓ Navigation ended with unknown status: {result.status}')

    def load_json_waypoints(self):
        '''Loads waypoints from JSON file and converts them into PoseStamped messages.'''
        if not os.path.exists(self.file_path):
            self.get_logger().error(f'❌ Waypoints file not found: {self.file_path}')
            return None

        with open(self.file_path, 'r') as f:
            data = json.load(f)

        waypoints_data = data.get('waypoints', [])
        if not waypoints_data:
            self.get_logger().warn('⚠️ No waypoints found in the JSON file.')
            return None

        waypoints = []
        for idx, wp in enumerate(waypoints_data):
            goal = PoseStamped()
            goal.header.frame_id = 'map'
            goal.header.stamp = self.get_clock().now().to_msg()
            goal.pose.position.x = wp['position']['x']
            goal.pose.position.y = wp['position']['y']
            goal.pose.position.z = wp['position']['z']
            goal.pose.orientation.x = wp['orientation']['x']
            goal.pose.orientation.y = wp['orientation']['y']
            goal.pose.orientation.z = wp['orientation']['z']
            goal.pose.orientation.w = wp['orientation']['w']
            waypoints.append(goal)
            self.get_logger().info(f'📍 Loaded Waypoint {idx+1}: ({goal.pose.position.x}, {goal.pose.position.y})')

        return waypoints

    def blackboard_callback(self, msg):
        '''
        Saves the blackboard data to a dictionary and extracts waypoints, saving them.
        Also publishes the status seen in the blackboard to the status topic.
        '''
        for entry in msg.data.strip().split('\n'):
            key, value = entry.split(': ', 1)
            self.blackboard[key] = value

        waypoints = []
        try:
            string_goals = self.blackboard["goals"].split('\n')[0].split('(')[1:]
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
                    'position': {'x': pos_x, 'y': pos_y, 'z': pos_z},
                    'orientation': {'x': ori_x, 'y': ori_y, 'z': ori_z, 'w': ori_w}
                })
            
        except Exception as e:
            self.get_logger().warn(f'Error in extracting waypoints: {e}')
            return None

        if waypoints:
            self.save_waypoints(waypoints)

    def save_waypoints(self, waypoints):
        ''' Saves the extracted waypoints to a JSON file. '''
        with open(self.file_path, 'w') as f:
            json.dump({'waypoints': waypoints}, f, indent=2)
        self.get_logger().info(f'📁 Waypoints saved to: {self.file_path}')

    def publish_waypoint_markers(self):
        '''Publishes waypoints as markers to RViz for visualization. Currently not used.'''
        marker_array = MarkerArray()

        for idx, goal in enumerate(self.waypoints):
            marker = Marker()
            marker.header.frame_id = 'map'
            marker.header.stamp = self.get_clock().now().to_msg()
            marker.ns = 'waypoints'
            marker.id = idx
            marker.type = Marker.ARROW  # arrow to indicate waypoints
            marker.action = Marker.ADD
            marker.pose = goal.pose  # Use the same pose as the waypoint
            marker.scale.x = 0.15  # Size of marker
            marker.scale.y = 0.15
            marker.scale.z = 0.15
            marker.color.r = 0.0  # Green
            marker.color.g = 1.0
            marker.color.b = 0.0
            marker.color.a = 1.0  # Fully visible

            text_marker = Marker()
            text_marker.header.frame_id = 'map'
            text_marker.header.stamp = self.get_clock().now().to_msg()
            text_marker.ns = 'waypoints_labels'
            text_marker.id = idx + 1000  # Offset to avoid ID conflict
            text_marker.type = Marker.TEXT_VIEW_FACING
            text_marker.action = Marker.ADD
            text_marker.pose = goal.pose
            text_marker.pose.position.z += 0.3  # Raise text above the marker
            text_marker.scale.z = 0.2  # Text size
            text_marker.color.r = 1.0  # White text
            text_marker.color.g = 1.0
            text_marker.color.b = 1.0
            text_marker.color.a = 1.0
            text_marker.text = f'wp_{idx+1}' # Label as 'wp_1', 'wp_2', etc.

            marker_array.markers.append(marker)
            marker_array.markers.append(text_marker)

        self.marker_pub.publish(marker_array)
        self.get_logger().info(f'📌 Published {len(self.waypoints)} waypoints to /waypoints for visualization.')


def main(args=None):
    '''Main function to start the ROS2 node.'''
    rclpy.init(args=args)
    node = WaypointNavigator()
    executor = MultiThreadedExecutor()
    executor.add_node(node)
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
