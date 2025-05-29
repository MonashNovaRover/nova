#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Node that loads waypoints from a file and 
sends it to the /urc_2025_navigator action
server. It continuously checks the status of the 
action server to monitor if it has been
aborted. If the action server has aborted, it will 
reload the waypoints to restart navigation.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: WaypointNavigator
ACTIONS: 
  - client: /urc_2025_navigator [URC2025Navigator]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	nova_utils
AUTHOR(S):	Tarik Thomas, Terry Tian, 
            Victor Bartlinski
CREATION:	15/03/2025
EDITED:		16/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from rclpy.client import Client
from rclpy.qos import QoSProfile, QoSDurabilityPolicy
from rclpy.executors import MultiThreadedExecutor
from std_msgs.msg import String
from geometry_msgs.msg import PoseStamped, Point
from geographic_msgs.msg import GeoPoint
from nav2_msgs.action import NavigateThroughPoses
from nova_interfaces.action import URC2025Navigator
from action_msgs.msg import GoalStatus
from visualization_msgs.msg import Marker, MarkerArray
from robot_localization.srv import FromLL
from tf2_ros import Buffer, TransformListener
from tf_transformations import quaternion_from_euler
import json
import os
import sys
import time
import math

# Goal types
# GNSS = 0
# AR = 1
# OBJECT = 2
GOAL_TYPES = {0: 'GNSS', 1: 'AR', 2: 'OBJECT'}

class WaypointNavigator(Node):
    def __init__(self):
        super().__init__('waypoint_navigator')

        # 📝 Populate parameters
        self._file_path = self.declare_parameter(
            name='file_path', 
            value=os.path.expanduser('~/nova/src/ros/rover/auto_bringup/params/waypoints.json'), 
        ).value
        self._gps = self.declare_parameter(
            name='gps', 
            value=True, 
        ).value
        self._lat = self.declare_parameter(
            name='lat', 
            value=38.41527178118493, 
        ).value
        self._lon = self.declare_parameter(
            name='lon', 
            value=-110.78853604626094, 
        ).value
        self._type = self.declare_parameter(
            name='type', 
            value=0, 
        ).value
        self._search_radius = self.declare_parameter(
            name='search_radius', 
            value=-1, 
        ).value
        self._goal_handle = None    # Prevents race condition with /fromLL service
        self._waypoints = None      # Prevents race condition with /fromLL service
        # 📝 Set default search radius based on goal type
        if self._search_radius == -1:
            if self._type == 1:
                self._search_radius = 20
            elif self._type == 2:
                self._search_radius = 10
            else:
                self._search_radius = 0
        
        # 📝 Create TF listener to get rover position in create_waypoint()
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)

        # 📝 Create action client for Nova URC2025Navigator
        self._action_client = ActionClient(self, URC2025Navigator, '/urc_2025_navigator')

        # 📝 Load waypoints from JSON
        self._waypoints = self.load_waypoints()

        if not self._waypoints:
            self.get_logger().error('❌ No waypoints found or failed to load JSON.')

            if not self._gps:
                return

            # 📝 Create service client for robot_localization FromLL
            self.get_logger().info('🗺️ Using GNSS coordinates instead.')
            self._fromll_client = self.create_client(FromLL, '/fromLL')

            # 📝 Wait for the robot_localization fromLL service
            self.get_logger().info('⏳ Waiting for /fromLL server...')
            if not self._fromll_client.wait_for_service(timeout_sec=10.0):
                self.get_logger().error('❌ FromLL server not available. Exiting.')
                return
            self.get_logger().info('✅ FromLL server available!')

            # 📝 Convert GNSS goal to Nav2 goal
            self.call_fromll_async()

        # 📝 Wait for the Nav2 action server
        self.get_logger().info('⏳ Waiting for /urc_2025_navigator action server...')
        if not self._action_client.wait_for_server(timeout_sec=10.0):
            self.get_logger().error('❌ Action server not available. Exiting.')
            return
        self.get_logger().info('✅ Action server available!')

        # 📝 Send waypoints asynchronously as an action goal
        if self._waypoints: # Prevents race condition with /fromLL service by calling after result_fromll_callback()
            self.send_goal_async()

        # 📝 Save waypoints
        self.subscription = self.create_subscription(String, '/blackboard', self.blackboard_callback, 1)
        self.get_logger().info('🚀 WaypointRecorder started! Listening to /blackboard...')

        # 📝 Create a timer to check navigation status every second
        self._nav_check_timer = self.create_timer(1.0, self.check_nav_status)

    def blackboard_callback(self, msg):
        ''' Extracts waypoints from the 'goals' section of the blackboard topic and saves them.'''
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
        with open(self._file_path, 'w') as f:
            json.dump({'waypoints': waypoints}, f, indent=2)
        self.get_logger().info(f'📁 Waypoints saved to: {self._file_path}')


    def load_waypoints(self):
        '''Loads waypoints from JSON file and converts them into PoseStamped messages.'''
        if not os.path.exists(self._file_path):
            self.get_logger().error(f'❌ Waypoints file not found: {self._file_path}')
            return None

        with open(self._file_path, 'r') as f:
            data = json.load(f)

        waypoints_data = data.get('waypoints', [])
        if not waypoints_data:
            self.get_logger().warn('⚠️ No waypoints found in the JSON file.')
            return None

        waypoints = []
        for idx, wp in enumerate(waypoints_data):
            pose = PoseStamped()
            pose.header.frame_id = 'map'
            pose.header.stamp = self.get_clock().now().to_msg()
            pose.pose.position.x = wp['position']['x']
            pose.pose.position.y = wp['position']['y']
            pose.pose.position.z = wp['position']['z']
            pose.pose.orientation.x = wp['orientation']['x']
            pose.pose.orientation.y = wp['orientation']['y']
            pose.pose.orientation.z = wp['orientation']['z']
            pose.pose.orientation.w = wp['orientation']['w']
            waypoints.append(pose)
            self.get_logger().info(f'📍 Loaded {GOAL_TYPES[self._type]} Waypoint {idx+1}: ({pose.pose.position.x}, {pose.pose.position.y})')

        return waypoints

    def create_waypoint(self, point):
        '''Creates a waypoint from a geometry_msgs/msg/Point.'''
        pose = PoseStamped()
        pose.header.frame_id = 'map'
        pose.header.stamp = self.get_clock().now().to_msg()
        pose.pose.position.x = point.x
        pose.pose.position.y = point.y
        pose.pose.position.z = 0.0

        # 📝 Get the rover's position in the map frame
        try:
            transform = self.tf_buffer.lookup_transform('map', 'base_link', rclpy.time.Time(), rclpy.time.Duration(seconds=10.0))
        except Exception as e:
            self.get_logger().error(f' ❌ Transform was not available! {e}')
            raise Exception
        rover = Point()
        rover.x = transform.transform.translation.x
        rover.y = transform.transform.translation.y

        # 📝 Calculate orientation for goal based on position relative to rover
        dx = point.x - rover.x
        dy = point.y - rover.y
        yaw = math.atan2(dy, dx)
        q = quaternion_from_euler(0, 0, yaw)
        pose.pose.orientation.x = q[0]
        pose.pose.orientation.y = q[1]
        pose.pose.orientation.z = q[2]
        pose.pose.orientation.w = q[3]

        # 📝 Save the waypoint to avoid race condition where the BT fails before waypoints are published to /blackboard
        waypoints = [{
            'position': {
                'x': pose.pose.position.x, 
                'y': pose.pose.position.y, 
                'z': pose.pose.position.z, 
            },
            'orientation': {
                'x': pose.pose.orientation.x, 
                'y': pose.pose.orientation.y, 
                'z': pose.pose.orientation.z, 
                'w': pose.pose.orientation.w, 
            }, 
        }]
        self.save_waypoints(waypoints)

        return [pose]

    def publish_waypoint_markers(self):
        '''Publishes waypoints as markers to RViz for visualization. Currently not used.'''
        marker_array = MarkerArray()

        for idx, pose in enumerate(self._waypoints):
            marker = Marker()
            marker.header.frame_id = 'map'
            marker.header.stamp = self.get_clock().now().to_msg()
            marker.ns = 'waypoints'
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
            text_marker.header.frame_id = 'map'
            text_marker.header.stamp = self.get_clock().now().to_msg()
            text_marker.ns = 'waypoints_labels'
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
            text_marker.text = f'wp_{idx+1}' # Label as 'wp_1', 'wp_2', etc.

            marker_array.markers.append(marker)
            marker_array.markers.append(text_marker)

        self.marker_pub.publish(marker_array)
        self.get_logger().info(f'📌 Published {len(self._waypoints)} waypoints to /waypoints for visualization.')


    def send_goal_async(self):
        '''Sends the waypoints asynchronously to the URC2025Navigator action server.'''
        goal_msg = URC2025Navigator.Goal()
        goal_msg.poses = self._waypoints  # List of PoseStamped
        goal_msg.type = self._type  # Type of goal (GNSS=0, AR=1, OBJECT=2)
        goal_msg.search_radius = self._search_radius  # Search radius for goal (GNSS is 0m, AR is 20m, OBJECT is 10m)
        self.get_logger().info('🚀 Sending waypoints to /urc_2025_navigator...')
        send_future = self._action_client.send_goal_async(goal_msg)
        send_future.add_done_callback(self.response_goal_callback)


    def response_goal_callback(self, future):
        goal_handle = future.result()
        if not goal_handle.accepted:
            self.get_logger().error('❌ Goal was rejected by the action server.')
            return

        self.get_logger().info('✅ Goal accepted. Navigating through waypoints...')
        self._goal_handle = goal_handle

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

    def call_fromll_async(self):
        '''Converts GNSS goal to a geometry_msgs/msg/Point using the robot_localization FromLL service.'''
        fromll_msg = FromLL.Request()
        fromll_msg.ll_point.latitude = self._lat
        fromll_msg.ll_point.longitude = self._lon
        self.get_logger().info('🚀 Sending GNSS goal to /fromLL...')
        send_future = self._fromll_client.call_async(fromll_msg)
        send_future.add_done_callback(self.result_fromll_callback)

    def result_fromll_callback(self, future):
        result = future.result()

        # 📝 Create waypoint
        try:
            self._waypoints = self.create_waypoint(result.map_point)
        except Exception:
            self.get_logger().error('❌ Failed to convert GNSS goal to Nav2 goal. Exiting.')
            return
        self.get_logger().info(f'📍 Loaded {GOAL_TYPES[self._type]} Waypoint 1: ({self._waypoints[0].pose.position.x}, {self._waypoints[0].pose.position.y})')
        self.send_goal_async()

    def check_nav_status(self):
        if not self._goal_handle:
            self.get_logger().warn('⚠️ No active goal handle. Skipping navigation status check.')
            return

        if self._goal_handle.status == GoalStatus.STATUS_ABORTED:
            self.get_logger().info('❌ Navigation aborted detected by timer callback!')
            self.get_logger().info('🚀 Sending waypoints to restart navigation')
            self._waypoints = self.load_waypoints()
            self.send_goal_async()


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