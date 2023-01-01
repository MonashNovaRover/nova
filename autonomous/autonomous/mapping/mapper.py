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
import rclpy
from rclpy.node import Node
from rclpy.time import Time
import numpy as np
import time
from autonomous.cameras.depth_camera import DepthCamera
from autonomous.config.runtime_params import max_point_depth, max_fov_angle, skip_pts, min_map_update_time
import logging

from tf2_ros import TransformListener, Buffer

class Mapper(Node):
    def __init__(self, length=20, width=20, height=5, planner=None, resolution=0.1, camera=False, name='mapper'):
        super().__init__(name)
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self, spin_thread=True)

        self.length = length
        self.width = width
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

        # if camera is true we create one, start it, and add a callback. Depth camera runs in a separate thread
        if camera:
            self.camera = DepthCamera(self.python_callback)
            self.camera.start()
        self.has_color = False

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
    def prune_point_cloud(pts, colors=None):
        """
        :param pts: np.array with shape (n, 3)
        :param colors: np.array with shape (n, 3)
        :return: np.array with shape (n, 3) or two such arrays as a tuple
        """
        # 1. only taking every 10th value (cos 2 much data)
        if len(pts) == 0:
            return pts
        pts = pts[::skip_pts]
        if colors:
            colors = colors[::skip_pts]

        # 2. Pruning out points which are either beyond the max dist, or are outside the max angle
        indexes = (Mapper.row_norm(pts) < max_point_depth) & (abs(np.arctan(pts[:, 1] / pts[:, 0])) < max_fov_angle) \
                  & (abs(np.arctan(pts[:, 2] / pts[:, 0])) < max_fov_angle)

        if colors:
            return pts[indexes], colors
        return pts[indexes]

    @staticmethod
    def convert_pts_to_tracking(pts):
        """
        Converts a numpy array of points from (x=right, y=down, z=forward) coordinates to (x=forward, y=right, z=up)
        :param pts: np.array with shape (n, 3), corresponding to an array of [x, y, z] coordinates
        :return: np.array with shape (n, 3)
        """
        pts = pts[:, [2, 0, 1]]
        pts[:, 2] = -pts[:, 2]
        pts[:, 1] = -pts[:, 1]
        return pts

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
        if time.perf_counter() - self.previous_map_update > min_map_update_time:
            self.handle_pc(pts)

            self.previous_map_update = time.perf_counter()

        self.planner.update_map(self.get_2d_map())

    def get_pts(self, pts):
        return self.prune_point_cloud(self.convert_pts_to_tracking(pts))

    def python_callback(self, pts):
        """
        This is called when the depth camera receives a new set of points via the python api. It is implemented
        as a callback so it can happen in a separate thread.
        It calls a function to extract and filter the points (colors are ignored) and updates the map with points only -
        when using the python API, it should be a points only map.
        """
        self.update_map(self.get_pts(pts))


def position_callback(msg):
    """
    Parses positional data, calculates the average value and publishes
    it to the topic /obstacle_proximity.
    """
    pass


def main(args=None):
    rclpy.init(args=args)
    # reset_cameras.reset_cameras()
    subscriber = Mapper()
    rclpy.spin(subscriber)
    subscriber.destroy_node()
    rclpy.shutdown()


def vis():
    """
    Example function loads a pre existing map into voxel memory and visualises it as a point-cloud
    """
    rclpy.init()
    m = Mapper(length=10, width=10, height=5, resolution=.2)
    m._map3d.grid2d = np.load("resources/environment.npy")
    m.publish_vis_dense(extra_pts=2)


if __name__ == '__main__':
    main()
