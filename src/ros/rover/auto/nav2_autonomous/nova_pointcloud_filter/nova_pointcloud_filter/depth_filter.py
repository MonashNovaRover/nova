#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Custom ROS2 node that filters the upper 
  and lower boundaries of a depth image in 
  order to prevent the generation of false-
  positive points at the borders.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: depth_filter
TOPICS:
  - subscriber: /oak/stereo/image_raw [Image]
  - publisher: /oak/stereo/image_filtered [Image]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_pointcloud_filter
AUTHOR(S):	Victor Bartlinski
CREATION:	03/04/2024
EDITED:		09/04/2024
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
from sensor_msgs.msg import Image

# standard python imports
import time

class DepthFilter(Node):
  """
  Filters pointcloud by changing the depth image via republishing .../stereo/image_raw with a filtered depth image.
  """
  def __init__(self):
    super().__init__('depth_filter')
    self.get_logger().set_level(logging.DEBUG)

    # Custom internal variables
    self.logged_values: bool = False

    # ROS subscribers
    self.sub = self.create_subscription(Image, '/depth/image', self.cb_sub, 10)

    # ROS publishers
    self.pub = self.create_publisher(Image, '/depth/image_filtered', 10)

    # ROS params
    self.declare_parameter('t_filter', 0)
    self.declare_parameter('r_filter', 0)
    self.declare_parameter('b_filter', 0)
    self.declare_parameter('l_filter', 0)

    timer_period = 0.1  # run the timer 10 times per second
    self.create_timer(timer_period, self.publisher)
    self.get_logger().info("Loaded node '/depth_filter' in container '/nova_pointcloud_filter'")

  def cb_sub(self, msg: Image) -> None:
    """
    Callback for the /oak/stereo/image_raw topic. Receives the stereos camera's depth image.
    """
    self.msg = msg

  def filter_depth(self) -> None:
    """
    Assigns a null depth value to a portion of the depth image's top and bottom rows.
    """
    # Define necessary values from Image msg
    height = self.msg.height
    width = self.msg.step
    t_filter: int = self.get_parameter('t_filter').get_parameter_value().integer_value
    r_filter: int = self.get_parameter('r_filter').get_parameter_value().integer_value
    b_filter: int = self.get_parameter('b_filter').get_parameter_value().integer_value
    l_filter: int = self.get_parameter('l_filter').get_parameter_value().integer_value
    try:
      # Overwrite depth values for image in the top rows with null depth values
      for row in range(0, t_filter):
        for col in range(width):
          self.msg.data[(row*width)+col] = 0
      # Overwrite depth values for image in the right columns with null depth values
      for row in range(0, height):
        for col in range(width-r_filter, width):
          self.msg.data[(row*width)+col] = 0
      # Overwrite depth values for image in the bottom rows with null depth values
      for row in range(height-b_filter, height):
        for col in range(width):
          self.msg.data[(row*width)+col] = 0
      # Overwrite depth values for image in the left columns with null depth values
      for row in range(0, height):
        for col in range(0, l_filter):
          self.msg.data[(row*width)+col] = 0
    except:
      self.get_logger().debug(f"row: {row}, col: {col}, i: {(row*width)+col}, height: {height}, width: {width}")

  def test_1(self) -> None:
    """
    Sets a portion of the depth image to a null depth value.
    """
    # Define necessary values from Image msg
    height = self.msg.height
    width = self.msg.width
    # Overwrite depth values for image
    for i in range((height*width*2)):
      self.msg.data[i] = 0
  
  def test_2(self) -> None:
    """
    Counter that checks the resolution of the depth image.
    """
    if not self.logged_values:
      total_pixels: int = 0
      try: 
        for i in range((self.msg.height*self.msg.width*2)):
          self.msg.data[i]
          total_pixels += 1
      except:
        self.get_logger().debug(f"total pixels exceeded")
      self.get_logger().debug(f"height: {self.msg.height}, width: {self.msg.width}, total pixels: {total_pixels}")


  def publisher(self):
    """
    Publishes the filtered depth image.
    """
    try:
      self.filter_depth()
      self.pub.publish(self.msg)
    except:
      pass


def main(args=None):
  rclpy.init(args=args)
  node = DepthFilter()
  rclpy.spin(node)
  node.destroy_node()
  rclpy.shutdown()


if __name__ == "__main__":
    main()
