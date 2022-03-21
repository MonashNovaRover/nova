__package__ = "autonomous"

from config.ros_config import main_frame

"""
2d map class for storing obstacles detected by our
obstacle detector, which we can navigate easily 
using A*. 
"""
from config.runtime_params import min_point_density, max_safe_obstacle, unseen_map_val, max_fov_angle, max_point_depth
import rclpy
from rclpy.node import Node
from height_mapper import get_obstacles
import numpy as np
from scipy.signal import convolve2d
import math_utils.transform as transform
from nav_msgs.msg import OccupancyGrid, MapMetaData
from geometry_msgs.msg import Pose
from std_msgs.msg import Header
from builtin_interfaces.msg import Time
from vis.grid_pub import GridPub

class Grid2D(Node):
    def __init__(self, length, width, resolution=0.1):
        """
        2D flattening of the 3D occupancy grid we use to visualise the map
        :param length: length in m in the x direction
        :param width: width in m in the y direction
        :param resolution: side length of one grid square in the final grid
        that we use to plan paths (meters)
        :param detection_resolution: finer resolution we use to detect obstacles more
        accurately before downscaling to a map we can plan on.
        """

        super().__init__("occupancy_grid_publisher")
        self.publisher = self.create_publisher(OccupancyGrid, "autonomous/occupancy_grid", 10)

        self.length = length
        self.width = width
        self.resolution = resolution
        # unseen areas of the map all have a slight cost 
        self.map = np.full((int(length / resolution), int(width / resolution)), 100 * unseen_map_val)

        self.grid_pub = GridPub()

    def roll_map(self, x_change, y_change):
        """
        Cut off the 5 meters of the map behind us and add a new 5 meters on the end
        """
        five_m_in_px = int(5/self.resolution)
        new_map = np.full((int(self.length / self.resolution), int(self.width / self.resolution)), 100 * unseen_map_val)
        if x_change == -1:
            new_map[five_m_in_px:, :] = self.map[:-five_m_in_px, :]
        elif x_change == 1:
            new_map[:-five_m_in_px, :] = self.map[five_m_in_px:, :]
        if y_change == -1:
            new_map[:, five_m_in_px:] = self.map[:, :-five_m_in_px]
        elif y_change == 1:
            new_map[:, :-five_m_in_px] = self.map[:, five_m_in_px:]
        self.map = new_map
                    
    def map_as_sequence(self):
        return self.map.transpose().flatten().astype(int).tolist()

    def publish_grid(self, offset):
        length = int(self.length / self.resolution)
        width = int(self.width / self.resolution)
        x = (-self.length / 2) + offset[0]
        y = (-self.width / 2) + offset[1]
        self.grid_pub.publish_grid(self.resolution, length, width, x, y, self.map_as_sequence())

    def get_full_indexes(self, points):
        """
        Scales points in meters to array indices in the full 2d map with the planning
        resolution, to store for planning.
        :param points: (n, 3) ndarray of coordinates in meters
        """
        indexes = (points / self.resolution)
        indexes += np.array([self.length / (2 * self.resolution), self.width / (2 * self.resolution), 0])
        return indexes.round().astype(int)

    def valid_index(self, index):
        return index > 0 and index < (self.length / self.resolution)

    def add_obstacles(self, msg, offset, obstacles):
        """
        Function to add a list of coordinates and their values to the 2d map. 
        :param msg: pose message giving the coordinates of the camera so we can translate the points
        :param obstacles: list of coordinates and values of associated obstacles. Coordinates have
                          have been rotated according to the pose of the rover but not translated.
        """
        diff = self.get_full_indexes(np.array([[msg.pose.pose.position.x - offset[0],
            msg.pose.pose.position.y - offset[1], 0]])).astype(int)
        obstacles = obstacles + diff
        obstacles = np.array([obs for i, obs in enumerate(obstacles) if obs[0] > 0
                and obs[1] > 0 and obs[0] < self.length/self.resolution and 
                obs[1] < self.length/self.resolution])
        if len(obstacles):
            self.map[obstacles[:, 0], obstacles[:, 1]] = obstacles[:, 2]

