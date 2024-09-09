#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Custom ROS2 node that determines the 
distance between the rover and a marker for the BT
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: goal_distance
TOPICS:
  - subscriber: /aruco_detections [ArucoDetection]
  - publisher: /marker_distance/distance [Float64]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_ar_tag
AUTHOR(S):	Victor Bartlinski
CREATION:	04/05/2024
EDITED:		04/05/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
  - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# ROS imports
import rclpy
import logging

from rclpy.node import Node
from rclpy.time import Time
from tf2_ros import Buffer, TransformListener

# msg types
from aruco_opencv_msgs.msg import ArucoDetection, MarkerPose
from geometry_msgs.msg import Pose2D
from std_msgs.msg import Float64

# standard python imports
import time

class MarkerDistance(Node):
  """
  """
  def __init__(self):
    super().__init__('marker_distance')
    self.get_logger().set_level(logging.DEBUG)

    # ROS subscribers
    self.sub = self.create_subscription(ArucoDetection, '/aruco_detections', self.cb_sub, 10)
    self.marker_pose:Pose2D = Pose2D()
    self.marker_pose.x:float = float('inf')
    self.marker_pose.y:float = float('inf')

    # ROS publishers
    self.pub = self.create_publisher(Float64, '/marker_distance/distance', 10)

    # ROS params
    self.declare_parameter('tag', 0)
    self.tag:int = self.get_parameter('tag').get_parameter_value().integer_value

    # ROS Tf2 stuff
    self.tf_buffer = Buffer()
    self.tf_listener = TransformListener(buffer=self.tf_buffer, node=self, spin_thread=True)
    self.rover_pose = Pose2D()

    timer_period = 0.1  # run the timer 10 times per second
    self.create_timer(timer_period, self.publisher)
    rover_pose_period = 1 / 30
    self.create_timer(rover_pose_period, self.cb_rover_pose)
    self.get_logger().info("Loaded node '/marker_distance' in container '/nova_ar_tag'")

  def cb_sub(self, msg: ArucoDetection) -> None:
    """
    """
    self.msg = msg

  def cb_rover_pose(self) -> None:
    """
    """
    try:
      base_link_tf = self.tf_buffer.lookup_transform("map", "base_link", Time()).transform
    except:
      pass
    else:
      self.rover_pose.x = base_link_tf.translation.x
      self.rover_pose.y = base_link_tf.translation.y

  def calculate_distance(self) -> Float64:
    """
    """
    if self.tag != self.get_parameter('tag').get_parameter_value().integer_value:
      self.get_logger().info("Resetting AR marker id and distance")
      self.tag = self.get_parameter('tag').get_parameter_value().integer_value
      self.marker_pose.x = float('inf')
      self.marker_pose.y = float('inf')
    distance:Float64 = Float64()
    for marker in self.msg.markers:
      if marker.marker_id == self.tag:
        self.marker_pose.x = marker.pose.position.x
        self.marker_pose.y = marker.pose.position.y
        break
    x:float = self.marker_pose.x - self.rover_pose.x
    y:float = self.marker_pose.y - self.rover_pose.y
    distance.data = (x**2 + y**2)**0.5
    return distance

  def publisher(self):
    """
    """
    try:
      distance:Float64 = self.calculate_distance()
      self.pub.publish(distance)
    except:
      pass


def main(args=None):
  rclpy.init(args=args)
  node = MarkerDistance()
  rclpy.spin(node)
  node.destroy_node()
  rclpy.shutdown()


if __name__ == "__main__":
    main()
