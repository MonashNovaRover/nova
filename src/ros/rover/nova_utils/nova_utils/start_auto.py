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
TOPICS:
    - subscriber: /blackboard                   [std_msgs/msg/String]
    - publisher: /auto/status             [nova_interfaces/msg/Status]
SERVICES:
    - client: /fromLL                           [robot_localization/srv/FromLL]
    - client: /set_RGBInput                     [nova_interfaces/srv/RGBInput]
    - service: /autonomous/cartographer_command [nova_interfaces/srv/CartographerCommand]
ACTIONS: 
  - client: /urc_2025_navigator                 [URC2025Navigator]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	nova_utils
AUTHOR(S):	Tarik Thomas, Terry Tian, 
            Victor Bartlinski
CREATION:	27/05/2025
EDITED:		29/05/2025
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
from geographic_msgs.msg import GeoPoint
from nav2_msgs.action import NavigateThroughPoses
from nova_interfaces.msg import Status
from nova_interfaces.action import URC2025Navigator
from nova_interfaces.srv import CartographerCommand, RGBInput
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
from typing import Tuple

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
        self._status_topic = self.declare_parameter(
            name='status_topic',
            value='/auto/status', 
        ).value
        self._ar_tag_search_radius = self.declare_parameter(
            name='ar_tag_search_radius',
            value=20,  # Default search radius for AR tags
        ).value
        self._object_search_radius = self.declare_parameter(
            name='object_search_radius',
            value=10,  # Default search radius for objects
        ).value
        
        self._search_radii = [0, self._ar_tag_search_radius, self._object_search_radius]
        self._blackboard = dict() # Dictionary to store blackboard data
        self._goal_handle = None

        # 📝 Create TF listener to get rover position in create_waypoint()
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)

        # 📝 Create action client for Nova URC2025Navigator
        self._action_client = ActionClient(self, URC2025Navigator, '/urc_2025_navigator')
        self.get_logger().info('⏳ Waiting for /urc_2025_navigator action server...')
        if not self._action_client.wait_for_server(timeout_sec=10.0):
            self.get_logger().error('❌ Action service /urc_2025_navigator not available. Exiting.')
            return
        self.get_logger().info('✅ Action service /urc_2025_navigator available!')

        # 📝 Create service client for robot_localization FromLL
        self._fromll_client = self.create_client(FromLL, '/fromLL')
        self.get_logger().info('⏳ Waiting for /fromLL server...')
        if not self._fromll_client.wait_for_service(timeout_sec=10.0):
            self.get_logger().error('❌ Service /fromLL not available. Exiting.')
            return
        self.get_logger().info('✅ Service /fromLL available!')

        # 📝 Create service client for LED control
        self._led_client = self.create_client(RGBInput, '/set_RGBInput')
        if not self._led_client.wait_for_service(timeout_sec=10.0):
            self.get_logger().error('❌ Service /set_RGBInput not available. Cannot change LED color.')
            return
        self.get_logger().info('✅ Service /set_RGBInput available!')

        # 📝 Create service for Auto GUI 
        self._cartographer_service = self.create_service(CartographerCommand, '/autonomous/cartographer_command', self.cartographer_callback)
        self.get_logger().info('✅ Service /autonomous/cartographer_command created!')

        # 📝 Save waypoints
        self._sub_blackboard = self.create_subscription(String, '/blackboard', self.blackboard_callback, QoSPresetProfiles.SENSOR_DATA.value)
        self.get_logger().info('✅ Subscriber /blackboard created!')

        # Create publisher for navigation status
        self._status_pub = self.create_publisher(Status, self._status_topic, QoSPresetProfiles.SENSOR_DATA.value)
        self.get_logger().info(f'✅ Publisher {self._status_topic} created!')


    def blackboard_callback(self, msg):
        '''
        Saves the blackboard data to a dictionary and extracts waypoints, saving them.
        Also publishes the status seen in the blackboard to the status topic.
        '''
        for entry in msg.data.strip().split('\n'):
            key, value = entry.split(': ', 1)
            self._blackboard[key] = value

        waypoints = []
        try:
            string_goals = self._blackboard["goals"].split('\n')[0].split('(')[1:]
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

        # Publish the status to the status topic
        status = int(self._blackboard.get('status', 0))  # Default to 0 if not found
        self.status_callback(status)


    def status_callback(self, status: int) -> None:
        '''Publishes the current navigation status to the status topic.'''
        status_msg = Status()
        status_msg.status = status
        self._status_pub.publish(status_msg)


    def cartographer_callback(self, request, response):
        try:
            # 📝 Process client request
            self._types = request.types
            self._goals = request.goals
            self._search_radius = self._search_radii[self._types[-1]]

            # 📝 Set the previous goal as the rover's current position in the map frame
            try:
                transform = self.tf_buffer.lookup_transform('map', 'base_link', rclpy.time.Time(), rclpy.time.Duration(seconds=10.0))
            except Exception as e:
                self.get_logger().error(f' ❌ Transform was not available! {e}')
                raise Exception
            self._prev_goal = Point()
            self._prev_goal.x = transform.transform.translation.x
            self._prev_goal.y = transform.transform.translation.y
            
            # 📝 Load waypoints from the Auto GUI
            self._waypoints = []
            self.load_gui_waypoints()

            response.success = True

        except Exception as e:
            self.get_logger().error(f'❌ Failed to process cartographer command: {e}')
            response.success = False

        return response

    def start_navigation(self):
        '''Starts the navigation by sending the waypoints to the action server.'''
        # 📝 Save waypoints to avoid race condition where the BT fails before waypoints are published to /blackboard
        waypoints = []
        for goal in self._waypoints:
            waypoints.append({
                'position': {
                    'x': goal.pose.position.x,
                    'y': goal.pose.position.y,
                    'z': goal.pose.position.z
                },
                'orientation': {
                    'x': goal.pose.orientation.x,
                    'y': goal.pose.orientation.y,
                    'z': goal.pose.orientation.z,
                    'w': goal.pose.orientation.w
                }
            })
        self.save_waypoints(waypoints)

        # 📝 Set LED to red to signify navigation start
        self.call_led_async((255, 0, 0))

        # 📝 Send waypoints asynchronously as an action goal
        self.send_goal_async()

        # 📝 Create a timer to check navigation status every second
        self._nav_check_timer = self.create_timer(1.0, self.check_nav_status)

    def load_gui_waypoints(self):
        '''Loads waypoints from GUI and converts them into PoseStamped messages.'''
        self.get_logger().info(f'⌛ {len(self._waypoints)}/{len(self._goals)} waypoints created...')
        if len(self._waypoints) >= len(self._goals):
            self.get_logger().info('✅ All waypoints loaded from GUI.')
            self.start_navigation()
            return

        try:
            # 📝 Convert GNSS goal to Nav2 goal
            goal = self._goals[len(self._waypoints)]
            self.call_fromll_async(goal.latitude, goal.longitude)
        except Exception as e:
            self.get_logger().error(f'❌ Failed to load GUI waypoints: {e}')

    def call_fromll_async(self, lat, lon):
        '''Converts GNSS goal to a geometry_msgs/msg/Point using the robot_localization FromLL service.'''
        fromll_req = FromLL.Request()
        fromll_req.ll_point.latitude = lat
        fromll_req.ll_point.longitude = lon
        self.get_logger().info(f'🚀 Sending GNSS goal {lat}, {lon} to /fromLL...')
        future = self._fromll_client.call_async(fromll_req)
        future.add_done_callback(self.result_fromll_callback)
    
    def result_fromll_callback(self, future):
        result = future.result()
        # 📝 Create waypoint
        try:
            wp = self.create_waypoint(result.map_point)
            self._waypoints.append(wp)
            i = len(self._waypoints) - 1  # Current waypoint index
            self.get_logger().info(f'📍 Loaded {GOAL_TYPES[self._types[i]]} Waypoint {i+1}: ({wp.pose.position.x}, {wp.pose.position.y})')
            self.load_gui_waypoints()  # Load next waypoint
        except Exception as e:
            self.get_logger().error(f'❌ Failed to convert GNSS goal to Nav2 goal: {e}')

    def create_waypoint(self, point):
        '''Creates a waypoint from a geometry_msgs/msg/Point.'''
        goal = PoseStamped()
        goal.header.frame_id = 'map'
        goal.header.stamp = self.get_clock().now().to_msg()
        goal.pose.position.x = point.x
        goal.pose.position.y = point.y
        goal.pose.position.z = 0.0

        # 📝 Calculate orientation for goal based on position relative to previous goal
        dx = point.x - self._prev_goal.x
        dy = point.y - self._prev_goal.y
        yaw = math.atan2(dy, dx)
        q = quaternion_from_euler(0, 0, yaw)
        goal.pose.orientation.x = q[0]
        goal.pose.orientation.y = q[1]
        goal.pose.orientation.z = q[2]
        goal.pose.orientation.w = q[3]

        # 📝 Update previous goal
        self._prev_goal = goal.pose.position

        return goal

    def call_led_async(self, rgb: Tuple[int, int, int], flash: bool = False) -> None:
        '''Calls the set_RGBInput service to change the LED color.'''
        request = RGBInput.Request()
        request.r = rgb[0]
        request.g = rgb[1]
        request.b = rgb[2]
        request.flash = flash
        self.get_logger().info(f'💡 Calling /set_RGBInput service to set LED color to ({rgb[0]}, {rgb[1]}, {rgb[2]}), flash = {flash}')
        future = self._led_client.call_async(request)
        future.add_done_callback(self.result_led_callback)

    def result_led_callback(self, future):
        '''Callback for the set_RGBInput service call.'''
        try:
            response = future.result()
            if response.success:
                self.get_logger().info('✅ LED color changed successfully!')
            else:
                self.get_logger().error('❌ Failed to change LED color.')
        except Exception as e:
            self.get_logger().error(f'❌ Service call failed: {e}')


    def send_goal_async(self):
        '''Sends the waypoints asynchronously to the URC2025Navigator action server.'''
        goal_msg = URC2025Navigator.Goal()
        goal_msg.poses = self._waypoints  # List of PoseStamped
        goal_msg.type = self._types[-1]  # Type of goal (GNSS=0, AR=1, OBJECT=2)
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
            # Publish ARRIVED status and change LED color to flashing green when navigation is successful
            self.status_callback(3)
            self.call_led_async((0, 255, 0), flash=True)
            self.get_logger().info('✅ Navigation through all waypoints succeeded!')
        elif result.status == GoalStatus.STATUS_CANCELED:
            self.get_logger().warn('⚠️ Navigation was canceled before completion.')
        elif result.status == GoalStatus.STATUS_ABORTED:
            self.get_logger().error('❌ Navigation failed.')
        else:
            self.get_logger().error(f'❓ Navigation ended with unknown status: {result.status}')

    def save_waypoints(self, waypoints):
        ''' Saves the extracted waypoints to a JSON file. '''
        with open(self._file_path, 'w') as f:
            json.dump({'waypoints': waypoints}, f, indent=2)
        self.get_logger().info(f'📁 Waypoints saved to: {self._file_path}')

    def load_json_waypoints(self):
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
            self.get_logger().info(f'📍 Loaded {GOAL_TYPES[self._types[idx]]} Waypoint {idx+1}: ({goal.pose.position.x}, {goal.pose.position.y})')

        return waypoints

    def publish_waypoint_markers(self):
        '''Publishes waypoints as markers to RViz for visualization. Currently not used.'''
        marker_array = MarkerArray()

        for idx, goal in enumerate(self._waypoints):
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
        self.get_logger().info(f'📌 Published {len(self._waypoints)} waypoints to /waypoints for visualization.')


    def check_nav_status(self):
        if not self._goal_handle:
            self.get_logger().warn('⚠️ No active goal handle. Skipping navigation status check.')
            return

        if self._goal_handle.status == GoalStatus.STATUS_ABORTED:
            self.get_logger().info('❌ Navigation aborted detected by timer callback!')
            self.get_logger().info('🚀 Sending waypoints to restart navigation')
            self._waypoints = self.load_json_waypoints()
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