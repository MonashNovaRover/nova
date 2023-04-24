#!/usr/bin/python3

__package__ = "autonomous"

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team. Base Mapper class that
maps the 2d surroundings by simply extracting
layers from the 3d map. Extended by other Mappers
with more evolved obstacle detection algorithms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: points_grid
TOPICS:
  - Subscriber: /camera/depth/color/points [sensor_msgs.msg.PointCloud2]
  - Subscriber: /t265/odom/sample [nav_msgs.msg.Odometry]

SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	autonomous
AUTHOR(S):	Lucas, Kelly, Kelvin, Amesh, Liam, Max
CREATION:	27/09/2021
EDITED:		17/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - a lot 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import numpy as np
import time
import logging

from autonomous.config.runtime_params import min_map_update_time
from autonomous.cameras.pc_converter import read_points

import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration
from tf2_ros import TransformListener, Buffer
from sensor_msgs.msg import PointCloud2

class Mapper(Node):
    def __init__(self, height=5, planner=None, resolution=0.1, name='mapper', camera=False):
        super().__init__(name)
        self.initialised = False
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self, spin_thread=True)
        if camera:
            self.sub_pointcloud = self.create_subscription(PointCloud2, "/depth_camera/d435_1/cloud", self.pointcloud_callback, 10)

        self.height = height
        self.resolution = resolution

        self.planner = planner

        self.previous_plan = time.perf_counter()
        self.previous_map_update = time.perf_counter()

        self.get_logger().set_level(logging.INFO)

        self.get_logger().info("Waiting for transform from 'local_map' to 'base_link'...")
        while not self.tf_buffer.can_transform('base_link', 'map', Time()):
            time.sleep(0.1)
        self.get_logger().info("Received Transform!")

        self.has_color = False

    def on_initialised(self):
        self.initialised = True

    def check_position_in_map(self):
        """
        Abstract method for use in inherited classes.
        """
        pass

    def initialise_map(self):
        """
        initialise the map
        """
        pass

    @staticmethod
    def row_norm(pts):
        """
        Efficient numpy way of doing euclidean distance over each row 
        (just takes all the values in the last index, which will be 3 for our purposes, and calculates the length)
        :param pts: (n, 3) array of points
        :return: what we need to
        """
        return np.sum(np.abs(pts) ** 2, axis=-1) ** 0.5

    def handle_pc(self, pts):
        """
        Abstract method
        """
        # transforming to the global frame
        raise NotImplemented("handle_pc must be implemented in a sub class")

    def get_2d_map(self):
        """
        Abstract method
        """
        raise NotImplemented("get_2d_map must be implemented in a sub class")

    def update_map(self, pts):
        """
        We only update the map every .5 seconds at most - need to take out magic number
        :param pts: np.array(n, 6) - refers to x,y,z,r,g,b
        """
        # transform the points
        self.get_logger().debug(f"Update map called after {time.perf_counter() - self.previous_map_update} s with {len(pts)} points", throttle_duration_sec=1)
        if time.perf_counter() - self.previous_map_update > min_map_update_time:
            self.get_logger().debug(f"handling pc: {pts}", throttle_duration_sec=1)
            self.handle_pc(pts)

            self.previous_map_update = time.perf_counter()

        self.planner.update_map(self.get_2d_map())

    def pointcloud_callback(self, msg):
        """
        This is called when the depth camera receives a new set of points via the python api. It is implemented
        as a callback so it can happen in a separate thread.
        It calls a function to extract and filter the points (colors are ignored) and updates the map with points only -
        when using the python API, it should be a points only map.
        """
        if not self.initialised:
            return
        self.get_logger().debug("Received pointcloud", throttle_duration_sec=1)
        self.update_map(np.array(list(read_points(msg, skip_nans=True))))


def main(args=None):
    rclpy.init(args=args)
    # reset_cameras.reset_cameras()
    subscriber = Mapper()
    rclpy.spin(subscriber)
    subscriber.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
