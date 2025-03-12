#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Republish blackboard String msg 
to show markers on BT goals
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: goal_marker
TOPICS:
  - subscriber: /blackboard [std_msgs/msg/String]
  - publisher: /goals       [visualization_msgs.MarkerArray]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_utils
EDITED BY:	Victor Bartlinski
CREATION:	08/03/2025
EDITED:		13/03/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration
from rclpy.qos import QoSHistoryPolicy, QoSDurabilityPolicy, QoSProfile

from visualization_msgs.msg import MarkerArray, Marker
from geometry_msgs.msg import Pose, Vector3, TransformStamped
from sensor_msgs.msg import Image, CameraInfo
from std_msgs.msg import String
from builtin_interfaces.msg import Time

from typing import Dict, List, Tuple, TypeVar
T = TypeVar('T')
type Point = Tuple[Tuple[float, float, float], Tuple[float, float, float, float]] # point = ((x,y,z), (X,Y,Z,W))

class GoalMarker(Node):
    def __init__(self):
        super().__init__('goal_marker')

        self.default_qos_profile = QoSProfile(
            reliability=1,
            history=QoSHistoryPolicy.KEEP_LAST,
            durability=QoSDurabilityPolicy.VOLATILE,
            depth=1,
        )

        self.map_frame = self.declare_parameter('map_frame', 'map').get_parameter_value().string_value
        self.marker_duration = self.declare_parameter('marker_duration', 1.0).get_parameter_value().double_value
        self.marker_ns = self.declare_parameter('marker_namespace', 'goal_marker').get_parameter_value().string_value
        self.marker_size = self.declare_parameter('marker_size', 0.15).get_parameter_value().double_value

        # subscribe to the topics
        self.blacboard_sub = self.create_subscription(
            String, "/blackboard", self.blacboard_sub_callback, qos_profile=self.default_qos_profile
        )
        # publish to the marker topic
        self.marker_pub = self.create_publisher(
            MarkerArray, '/goals', 10
        )


    def blacboard_sub_callback(self, str_msg: String) -> None:
        string:str = str_msg.data
        goals = []
        try:
            string_goals = string.split('goals: ')[1].split('\n')[0].split('(')[1:]
            for string_goal in string_goals:
                coords = string_goal.split(')')[0].split(', ')
                goals.append(((float(coords[0]), float(coords[1]), float(coords[2])), (float(coords[3]), float(coords[4]), float(coords[5]), float(coords[6]))))
        except Exception as e:
            self.get_logger().debug(f'Error in publishing marker goal: {e}')
            return None

        marker_msg = MarkerArray()
        for i, goal in enumerate(goals):
            marker = self.get_marker(i, goal, 'map')
            marker_msg.markers.append(marker)

        self.marker_pub.publish(marker_msg)
        return None


    def get_marker(self, id:int, point: Point, frame:str) -> Marker:
        '''Returns a marker derived from the detection'''
        marker = Marker()
        marker.pose.position.x, marker.pose.position.y, marker.pose.position.z = point[0]
        marker.pose.orientation.x, marker.pose.orientation.y, marker.pose.orientation.z, marker.pose.orientation.w = point[1]

        marker.type = Marker.ARROW
        marker.scale.x = self.marker_size * 3.0
        marker.scale.y = self.marker_size * 0.5
        marker.scale.z = self.marker_size * 0.5
        marker.color.r = 0.0
        marker.color.g = 1.0
        marker.color.b = 0.0
        marker.color.a = 1.0

        marker.lifetime = Duration(seconds=self.marker_duration).to_msg()
        marker.ns = self.marker_ns
        marker.id = id

        time = self.get_clock().now()
        stamp = Time()
        stamp.sec, stamp.nanosec = time.seconds_nanoseconds()

        marker.header.stamp = stamp
        marker.header.frame_id = frame

        return marker


if __name__ == '__main__':
    rclpy.init()
    node = GoalMarker()
    rclpy.spin(node)
    rclpy.shutdown()