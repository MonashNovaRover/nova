#!usr/bin/python3
__package__ = "autonomous"
"""
2d map class for storing obstacles detected by our
obstacle detector, which we can navigate easily 
using A*. 
"""
from config.runtime_params import unseen_map_val
from rclpy.node import Node
import numpy as np
from nav_msgs.msg import OccupancyGrid, MapMetaData
from config.ros_config import occupancy_grid_topic
from vis.grid_pub import GridPub
from rclpy.qos import qos_profile_sensor_data as qos
import logging


class Grid2D(Node): 
    def __init__(self, length: int, width: int, resolution=0.1):
        """
        2D flattening of the 3D occupancy grid we use to visualise the map
        :param length: length in m in the x direction
        :param width: width in m in the y direction
        :param resolution: side length of one grid square in the final grid
        that we use to plan paths (meters)
        accurately before downscaling to a map we can plan on.
        """

        super().__init__("grid_2d")
        self.length = length
        self.width = width
        self.resolution = resolution
        # unseen areas of the map all have a slight cost 
        self.map = np.full((int(length / resolution), int(width / resolution)), 100 * unseen_map_val)

        self.get_logger().set_level(logging.DEBUG)

        self.grid_pub = GridPub()

    def roll_map(self, x_change, y_change):
        """
        Cut off the 5 meters of the map behind us and add a new 5 meters on the end
        """
        edge_dist_in_px = int(self.length/(4 * self.resolution))
        new_map = np.full((int(self.length / self.resolution), int(self.width / self.resolution)), 100 * unseen_map_val)
        if x_change == -1:
            new_map[edge_dist_in_px:, :] = self.map[:-edge_dist_in_px, :]
        elif x_change == 1:
            new_map[:-edge_dist_in_px, :] = self.map[edge_dist_in_px:, :]
        if y_change == -1:
            new_map[:, edge_dist_in_px:] = self.map[:, :-edge_dist_in_px]
        elif y_change == 1:
            new_map[:, :-edge_dist_in_px] = self.map[:, edge_dist_in_px:]
        self.map = new_map

    @staticmethod
    def map_from_occupancy(grid: OccupancyGrid, offset: tuple, resolution: float, width: int, length: int) -> np.array:
        """
        This method effectively aims to re-create a self.map using what this node would be publishing as OccupancyGrid,
        so any subscribers to the occupancy_grid_topic can use this method to create an equivalent self.map
        :param: OccupancyGrid which
        """
        pass

    def map_as_sequence(self):
        # have to get all values between 0 and 100 without changing the map we store
        temp_map = np.copy(self.map)
        temp_map[temp_map > 100] = 100
        return temp_map.transpose().flatten().astype(int).tolist()

    def publish_grid(self):
        length = int(self.length / self.resolution)
        width = int(self.width / self.resolution)
        x = -self.length / 2
        y = -self.width / 2
        self.get_logger().debug("Publishing grid...")
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
        return 0 < index < (self.length / self.resolution)

    def add_obstacles(self, transform, obstacles):
        """
        Function to add a list of coordinates and their values to the 2d map. 
        :param msg: pose message giving the coordinates of the camera so we can translate the points
        :param obstacles: list of coordinates and values of associated obstacles. Coordinates have
                          have been rotated according to the pose of the rover but not translated.
        """
        if obstacles is None: return
        diff = self.get_full_indexes(np.array([[transform.translation.x, transform.translation.y, 0]])).astype(int)
        obstacles = obstacles + diff
        obstacles = np.array([obs for obs in obstacles if 0 < obs[0] < self.length / self.resolution
                              and 0 < obs[1] < self.width / self.resolution])
        if len(obstacles) > 0:
            self.map[obstacles[:, 0], obstacles[:, 1]] = obstacles[:, 2]

