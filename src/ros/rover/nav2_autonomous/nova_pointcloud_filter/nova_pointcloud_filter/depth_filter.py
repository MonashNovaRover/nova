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
    super().__init__("depth_filter")
    self.get_logger().set_level(logging.DEBUG)

    # Custom internal variables
    self.logged_values: bool = False

    # ROS Subscribers
    self.sub = self.create_subscription(Image, "/oak/stereo/image_raw", self.cb_sub, 10)
    time.sleep(0.11) # Gives node time to save the subscription topic before it tries to access it

    # ROS publishers
    self.pub = self.create_publisher(Image, "/oak/stereo/image_filtered", 10)

    timer_period = 0.1  # run the timer 10 times per second
    self.create_timer(timer_period, self.publisher)

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
    top_rows_to_remove: int = int(0 * height)
    bot_rows_to_remove: int = int(0.175 * height)
    try:
      # Overwrite depth values for image in the top rows with null depth values
      for row in range(0, top_rows_to_remove):
        for col in range(width):
          self.msg.data[(row*width)+col] = 0
      # Overwrite depth values for image in the bottom rows with null depth values
      for row in range(height-bot_rows_to_remove, height):
        for col in range(width):
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
    self.filter_depth()
    self.pub.publish(self.msg)


def main(args=None):
  rclpy.init(args=args)
  node = DepthFilter()
  rclpy.spin(node)
  node.destroy_node()
  rclpy.shutdown()


if __name__ == "__main__":
    main()
