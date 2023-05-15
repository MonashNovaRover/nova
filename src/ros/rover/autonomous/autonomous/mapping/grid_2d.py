#!usr/bin/python3
"""
2d map class for storing obstacles detected by our
obstacle detector, which we can navigate easily 
using A*. 
"""
from autonomous.config.runtime_params import unseen_map_val

from rclpy.node import Node

from nav_msgs.msg import OccupancyGrid

import numpy as np
import logging, math


class Grid2D(Node): 
    def __init__(self, length: float = 20.0, width: float = 20.0, resolution=0.1, outer_length = 20., outer_width=20., with_border=False):
        """
        2D flattening of the 3D occupancy grid we use to visualise the map
        :param length: length in m in the x direction
        :param width: width in m in the y direction
        :param resolution: side length of one grid square in the final grid
        that we use to plan paths (meters)
        accurately before downscaling to a map we can plan on.
        """
        super().__init__("grid_2d")
        self.get_logger().set_level(logging.INFO)
        self.param_tf_sub_hz = self.declare_parameter("tf_sub_frequency_hz", 30).value

        self.outer_length = outer_length
        self.outer_width = outer_width
        self.length = length
        self.width = width
        self.resolution = resolution

        # unseen areas of the map all have a slight cost 
        self.map = np.full((int(outer_length / resolution), int(outer_width / resolution)), 100 * unseen_map_val)
        self.with_border = with_border

        if with_border:
            self.inner_map_border_mask = self.calculate_inner_map_border_mask()

    def calculate_inner_map_border_mask(self):
        """
        Calculate a boolean array of all points which are in the boundary of the inner map
        """
        min_i = int(math.floor((self.outer_length - self.length) / (2 * self.resolution)))
        min_j = int(math.floor((self.outer_width - self.width) / (2 * self.resolution)))
        max_i = int(math.ceil((self.outer_length + self.length) / (2 * self.resolution)))
        max_j = int(math.ceil((self.outer_width + self.width) / (2 * self.resolution)))

        outer_width_index = int(self.outer_width / self.resolution)
        outer_length_index = int(self.outer_length / self.resolution)

        # 2 boxes wide so we definitely see the obstacle
        outer_edge = np.array([[j > min_j - 1 and j < max_j + 1 and i > min_i - 1 and i < max_i + 1 for j in range(outer_width_index)] for i in range(outer_length_index)])
        inner_edge = np.array([[j > min_j + 1 and j < max_j - 1 and i > min_i + 1 and i < max_i - 1 for j in range(outer_width_index)] for i in range(outer_length_index)])

        # Things that are in the outer region and not in the inner region gives us the border between them
        return np.logical_and(outer_edge, ~inner_edge)

    def roll_map(self, x_change, y_change):
        """
        Shift the map across by x_change meters in x direction and y_change meters in y direction
        """
        self.get_logger().debug(f"Rolling map: dx = {x_change}, dy = {y_change}")
        x_change_px = int(abs(x_change) / self.resolution)
        y_change_px = int(abs(y_change) / self.resolution)
        new_map = np.full((int(self.length / self.resolution), int(self.width / self.resolution)), 100 * unseen_map_val)
        if x_change < 0:
            new_map[x_change_px:, :] = self.map[:-x_change_px, :]
        elif x_change > 0:
            new_map[:-x_change_px, :] = self.map[x_change_px:, :]
        if y_change < 0:
            new_map[:, y_change_px:] = self.map[:, :-y_change_px]
        elif y_change > 0:
            new_map[:, :-y_change_px] = self.map[:, y_change_px:]
        self.map = new_map

    @staticmethod
    def map_from_occupancy(grid: OccupancyGrid, offset: tuple, resolution: float, width: int, length: int) -> np.array:
        """
        This method effectively aims to re-create a self.map using what this node would be publishing as OccupancyGrid,
        so any subscribers to the occupancy_grid_topic can use this method to create an equivalent self.map
        :param: OccupancyGrid which
        """
        return np.asanyarray(grid.data, dtype=np.int8)\
            .reshape((grid.info.width, grid.info.height))\
            .astype(float) / 100

    def as_bytes(self):
        # have to get all values between 0 and 100 without changing the map we store
        temp_map = np.copy(self.map)
        temp_map[0][0] = 101
        #temp_map[temp_map > 100] = 100
        return temp_map.transpose().flatten().astype(int).tolist()

    def get_full_indexes(self, points):
        """
        Scales points in meters to array indices in the full 2d map with the planning
        resolution, to store for planning.
        :param points: (n, 3) ndarray of coordinates in meters
        """
        indexes = (points / self.resolution)
        indexes += np.array([self.outer_length / (2 * self.resolution), self.outer_width / (2 * self.resolution), 0])
        return indexes.round().astype(int)

    def bound_inner_map(self):
        """
        Manually sets to 1.0 all points around the boundary of the inner map,
        centered at the midpoint of self.map, with a length of self.length and a width of self.width
        """
        self.map[self.inner_map_border_mask] = 100

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
        obstacles = np.array([obs for obs in obstacles if 0 < obs[0] < int(self.outer_length / self.resolution)
                              and 0 < obs[1] < int(self.outer_width / self.resolution)])
        if len(obstacles) > 0:
            self.map[obstacles[:, 0], obstacles[:, 1]] = obstacles[:, 2]
        
        if self.with_border:
            self.bound_inner_map()