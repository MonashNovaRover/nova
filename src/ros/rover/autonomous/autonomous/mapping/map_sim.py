#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Uses the rover's simulated position to
            get a section of a pre-created map and
            publish it over ros
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: map_sim_node
TOPICS:
  - subscriber: /tf [TransformStamped]
  - publisher: /autonomous/occupancy_grid [String]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Max Tory, Taaj Street
CREATION:   27/05/2023
EDITED:		27/05/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - change all the template artefacts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
# ros imports
import rclpy
from rclpy.node import Node
from rclpy.task import Future
from rclpy.time import Time
from tf2_ros import TransformListener, Buffer

# local imports

# message types
from nav_msgs.msg import OccupancyGrid, MapMetaData
from geometry_msgs.msg import Transform

# python imports
import cv2
import logging
import numpy as np


class SimulatedMapper(Node):

    def __init__(self):
        super().__init__("TemplateNode")
        self.get_logger().set_level(logging.INFO)
        self.param_full_map_path = self.declare_parameter("full_map_path", "map.png").value
        self.param_map_pub_hz = self.declare_parameter("map_pub_rate_hz", 10).value
        self.param_map_resolution = self.declare_parameter("map_resolution_m", 0.1).value
        self.param_local_map_width = self.declare_parameter("local_map_width_px", 200).value
        self.param_local_map_len = self.declare_parameter("local_map_len_px", 200).value

        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self, spin_thread=True)

        self.global_map = None
        self.global_map_len = -1
        self.global_map_width = -1

        self.load_initial_map()

        self.get_logger().info("Waiting for transform from map to local_map...")
        future : Future = self.tf_buffer.wait_for_transform_async("local_map", "map", rclpy.time.Time())
        future.add_done_callback(self.complete_node)

    def complete_node(self, future: Future):
        try:
            future.result()
        except Exception as e:
            self.get_logger().error(f"Failed to get transform: {e}")
            self.destroy_node()
        else:
            self.get_logger().info("Received Transform!")
        self.pub_map = self.create_publisher(OccupancyGrid, "/autonomous/occupancy_grid", 10)
        self.create_timer(1 / self.param_map_pub_hz, self.callback_pub_map)

    def load_initial_map(self):
        """
        Loads the map from the param_full_map_path
        :return: None
        """
        self.global_map = cv2.imread(self.param_full_map_path, 0)
        self.global_map = 1 - self.global_map / 255
        self.global_map = (self.global_map * 100).astype(np.ubyte)
        self.global_map_len = self.global_map.shape[0]
        self.global_map_width = self.global_map.shape[1]

    def construct_occupancy_grid(self, local_map):
        """
        Construct an occupancy grid message type from the local map array
        """
        msg = OccupancyGrid()
        msg.header.frame_id = "local_map"
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.info.resolution = self.param_map_resolution
        msg.info.width = self.param_local_map_len
        msg.info.height = self.param_local_map_width
        msg.info.map_load_time = self.get_clock().now().to_msg()
        msg.info.origin.position.x = -self.param_local_map_len / 2 * self.param_map_resolution
        msg.info.origin.position.y = -self.param_local_map_width / 2 * self.param_map_resolution
        msg.info.origin.position.z = 0.
        msg.info.origin.orientation.x = 0.
        msg.info.origin.orientation.y = 0.
        msg.info.origin.orientation.z = 0.
        msg.info.origin.orientation.w = 1.
        msg.data = local_map.transpose().flatten().tolist()
        return msg

    def callback_pub_map(self):
        """
        Called every timer_period. Publishes to self.publisher
        :return:
        """
        try:
            transform : Transform = self.tf_buffer.lookup_transform("map", "local_map", Time()).transform
        except:
            self.get_logger().warn("No transform from map to local_map")
            return
        
        x_pixels = self.global_map_len / 2 + transform.translation.x / self.param_map_resolution
        y_pixels = self.global_map_width / 2 + transform.translation.y / self.param_map_resolution
        self.get_logger().debug(f"Map offset: {x_pixels}, {y_pixels}")

        # pixel boundaries of the global map in the local map (if we are over the edge)
        local_lower_x = 0
        local_lower_y = 0
        local_upper_x = self.param_local_map_len
        local_upper_y = self.param_local_map_width

        # pixel boundaries of the area we can see in the global map
        lower_x = int(x_pixels - self.param_local_map_len / 2)
        lower_y = int(y_pixels - self.param_local_map_width / 2)
        upper_x = int(x_pixels + self.param_local_map_len / 2)
        upper_y = int(y_pixels + self.param_local_map_width / 2)

        if lower_x < 0:
            local_lower_x = -lower_x
            lower_x = 0
        if lower_y < 0:
            local_lower_y = -lower_y
            lower_y = 0
        if upper_x > self.global_map_len:
            local_upper_x = -upper_x + self.global_map_len
            upper_x = self.global_map_len
        if upper_y > self.global_map_width:
            local_upper_y = -upper_y + self.global_map_width
            upper_y = self.global_map_width

        local_map = np.full((self.param_local_map_len, self.param_local_map_width), -1)
        if lower_x < upper_x + self.param_local_map_len and lower_y < upper_y + self.param_local_map_width:
            local_map[local_lower_x:local_upper_x, local_lower_y:local_upper_y] = self.global_map[lower_x:upper_x, lower_y:upper_y]
        
        # convert to occupancy grid
        self.pub_map.publish(self.construct_occupancy_grid(local_map))


def main():
    rclpy.init()
    node = SimulatedMapper()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
