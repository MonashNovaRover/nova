__package__ = "autonomous"

from config.ros_config import main_frame

"""
2d map class for storing obstacles detected by our
obstacle detector, which we can navigate easily 
using A*. 
"""
from config.runtime_params import min_point_density, max_safe_obstacle
from height_mapper import get_obstacles
import numpy as np
from scipy.signal import convolve2d
import math_utils.transform as transform
from config.runtime_params import max_fov_angle, max_point_depth
from rclpy.node import Node
from nav_msgs.msg import OccupancyGrid, MapMetaData
from geometry_msgs.msg import Pose
from std_msgs.msg import Header
from builtin_interfaces.msg import Time
import matplotlib.pyplot as plt
from vis.grid_pub import GridPub

class Grid2D(Node):
    def __init__(self, length, width, resolution=0.1):
        """
        2D flattening of the 3D occupancy grid we use to visualise the map
        :param length: length in m in the x direction
        :param width: width in m in the y direction
        :param planning_resolution: side length of one grid square in the final grid
        that we use to plan paths (meters)
        :param detection_resolution: finer resolution we use to detect obstacles more
        accurately before downscaling to a map we can plan on.
        """

        super().__init__("occupancy_grid_publisher")
        self.publisher = self.create_publisher(OccupancyGrid, "autonomous/occupancy_grid", 10)

        self.length = length
        self.width = width
        self.resolution = resolution
        self.map = np.zeros((int(length / planning_resolution), int(width / planning_resolution)))

        self.grid_pub = GridPub()

    def map_as_sequence(self):
        return self.map.transpose().flatten().astype(int).tolist()

    def publish_grid(self):
        length = int(self.length / self.resolution)
        width = int(self.width / self.resolution)
        x = (-self.length / 2)
        y = (-self.width / 2)
        self.grid_pub.publish_grid(0.4 * self.resolution, length, width, 0.4 * x, 0.4 * y, self.map_as_sequence())

    def get_detection_map_indexes(self, points):
        """
        Scales points in meters to array indices in the sub-section of the grid that contains
        the new set of points. indices are scaled by the detection resolution.
        Z coordinates are centred on z = 128 as the cpp obstacle detection algorithm works
        with unsigned chars, so this will put it in the middle.
        :param points: array of points (x, y, z) in meters. Points have been translated
        to account for the rover's pitch and roll (so the same z coordinates are actually
        above one another), but yaw and position transformations are done after obstacle
        detection, so the obstacle detection map can stay a consistent size and shape
        """
        indexes = (points/self.detection_resolution).round().astype(int)
        indexes[:, 1] += (np.ceil(self.detection_map_width/2)).astype(int)
        return indexes

    def get_full_indexes(self, points):
        """
        Scales points in meters to array indices in the full 2d map with the planning
        resolution, to store for planning.
        :param points: (n, 3) ndarray of coordinates in meters
        """
        print("position = " + str(points))
        indexes = (points / self.planning_resolution)
        indexes += np.array([self.length / (2 * self.planning_resolution), self.width / (2 * self.planning_resolution), 0])
        return indexes.round().astype(int)

    def add_obstacles(self, msg, obstacles):
        """
        Function to add a list of coordinates and their values to the 2d map. 
        :param msg: pose message giving the coordinates of the camera so we can translate the points
        :param obstacles: list of coordinates and values of associated obstacles. Coordinates have
                          have been rotated according to the pose of the rover but not translated.
        """
        diff = self.get_full_indexes(np.array([[msg.pose.pose.position.x,
            msg.pose.pose.position.y, 0]])).astype(int)
        print("diff = " + str(diff))
        obstacles = obstacles + diff
        self.map[obstacles[:, 0], obstacles[:, 1]] = obstacles[:, 2]
        plt.figure()
        plt.imshow(self.map)
        plt.plot(diff[0, 0], diff[0, 1], 'ro')
        plt.savefig('map.png')

