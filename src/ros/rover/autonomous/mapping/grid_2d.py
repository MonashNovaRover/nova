__package__ = "autonomous"
"""
2d map class for storing obstacles detected by our
obstacle detector, which we can navigate easily 
using A*. 
"""
from config.runtime_params import min_point_density
from height_mapper import get_obstacles
import numpy as np
from scipy.signal import convolve2d
import math_utils.transform as transform
from config.runtime_params import max_fov_angle, max_point_depth

class Grid2D:
    def __init__(self, length, width, planning_resolution=0.1, detection_resolution=0.025):
        """
        2D flattening of the 3D occupancy grid we use to visualise the map
        :param length: length in m in the x direction
        :param width: width in m in the y direction
        :param planning_resolution: side length of one grid square in the final grid
        that we use to plan paths (meters)
        :param detection_resolution: finer resolution we use to detect obstacles more
        accurately before downscaling to a map we can plan on.
        """
        assert(planning_resolution >= detection_resolution)

        self.length = length
        self.width = width
        self.planning_resolution = planning_resolution
        self.detection_resolution = detection_resolution
        # defining L and W of the small map we use to detect obstacles in c++
        self.detection_map_length = int(np.ceil(max_point_depth / self.detection_resolution))
        self.detection_map_width = int(np.ceil(2 * max_point_depth * np.tan(max_fov_angle)))
        
        self.map = np.zeros((int(length / planning_resolution), int(width / planning_resolution)))

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
        print(points)
        indexes = (points/self.detection_resolution).round().astype(int)
        indexes[:, 1] += (np.ceil(self.detection_map_width/2)).astype(int)
        return indexes

    def get_full_indexes(self, points):
        """
        Scales points in meters to array indices in the full 2d map with the planning
        resolution, to store for planning.
        :param points: (n, 3) ndarray of coordinates in meters
        """
        indexes = (points / self.planning_resolution).round().astype(int)
        indexes[:, 2] = points[:, 2]
        indexes -= np.array([self.length / 2, self.width / 2, 0]).round().astype(int)
        return indexes

    def filter_points(self, points):
        """
        Discretises point cloud into indices, then filters out indices without
        enough points in them to avoid phantom "floating" points
        """
        indexes = self.get_detection_map_indexes(points)
        indexes, counts = np.unique(indexes, return_counts=True, axis=0)
        counts = (counts // min_point_density).astype(bool) # filtering out voxels without many points in them
        return indexes[counts]

    def downscale_obs(self, obstacles):
        """
        Uses convolution with a kernel of ones to add up the values in sections
        of the grid so that we can down-size the resolution.
        """
        scale_factor = int(self.planning_resolution/self.detection_resolution)
        kernel = np.ones((scale_factor, scale_factor))
        return convolve2d(obstacles, kernel, mode='valid')[::scale_factor, ::scale_factor]

    def pc_to_obstacles(self, points):
        """
        Gets point cloud, sorts out "noise" points, then passes it to the obstacle detector
        and stores the resulting 2d map of obstacles
        :param points: (n, 3) numpy array
        """
        indexes = self.filter_points(points)

        # cpp function finds steep areas in the high resolution map
        obstacles = get_obstacles(indexes)

        # Using scipy convolution to get a down-sampled array of obstacles
        return self.downscale_obs(obstacles) 

    def add_obstacles(self, msg, obstacles):
        """
        Function to add a list of coordinates and their values to the 2d map. 
        """
        diff = self.get_full_indexes(np.array([[msg.pose.pose.position.x,
            msg.pose.pose.position.y, 0]]))
        obstacles += diff
        print(obstacles)
        self.map[obstacles[:, 0], obstacles[:, 1]] = obstacles[:, 2] 

