#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Custom ROS2 node that publishes AR tag 
markers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: aruco_marker
TOPICS:
  - subscriber: /aruco_detections [ArucoDetection]
  - publisher: /aruco_marker/markers [MarkerArray]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_ar_tag
AUTHOR(S):	Victor Bartlinski
CREATION:	18/04/2024
EDITED:		18/04/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
  - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# ros imports
import rclpy
import logging

from rclpy.node import Node

# msg types
from aruco_opencv_msgs.msg import ArucoDetection, MarkerPose
from visualization_msgs.msg import Marker, MarkerArray
from geometry_msgs.msg import Pose
from std_msgs.msg import ColorRGBA

# standard python imports
import time

class ArucoMarker(Node):
  """
  """
  def __init__(self):
    super().__init__('aruco_marker')
    self.get_logger().set_level(logging.DEBUG)

    # Custom internal variables
    self.logged_values: bool = False

    # ROS Subscribers
    self.sub = self.create_subscription(ArucoDetection, '/aruco_detections', self.cb_sub, 10)

    # ROS publishers
    self.pub = self.create_publisher(MarkerArray, '/aruco_marker/markers', 10)

    timer_period = 0.1  # run the timer 10 times per second
    self.create_timer(timer_period, self.publisher)
    self.get_logger().info("Loaded node '/aruco_marker' in container '/nova_ar_tag'")

  def cb_sub(self, msg: ArucoDetection) -> None:
    """
    Callback for the /tf topic. Receives all transforms.
    """
    self.msg = msg

  def create_marker(self) -> MarkerArray:
    """
    """
    all_markers = MarkerArray()
    visible_markers = self.msg.markers
    for marker in visible_markers:
      msg = Marker()
      msg.pose = marker.pose
      msg.type = Marker.SPHERE
      msg.scale.x = .1
      msg.scale.y = .1
      msg.scale.z = .1
      color = ColorRGBA()
      color.r = 1.
      color.g = 1.
      color.b = 1.
      color.a = 1.
      msg.color = color
      msg.header.frame_id = self.msg.header.frame_id
      msg.header.stamp = self.msg.header.stamp
      msg.ns = "placeholder_namespace"
      msg.id = marker.marker_id
      all_markers.markers.append(msg)
    return all_markers

  def publisher(self):
    """
    Publishes the filtered depth image.
    """
    try:
      self.all_markers = self.create_marker()
      self.pub.publish(self.all_markers)
    except:
      pass


def main(args=None):
  rclpy.init(args=args)
  node = ArucoMarker()
  rclpy.spin(node)
  node.destroy_node()
  rclpy.shutdown()


if __name__ == "__main__":
    main()
