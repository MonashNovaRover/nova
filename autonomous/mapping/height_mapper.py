__package__ = "autonomous"
#!/usr/bin/python3
  

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team. Child class of the Mapper 
class that maps the 2d surroundings by simply 
fitting height maps over the maximum and minimum
values of the most recent section of the 3d point 
cloud sent by the Rover. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: points_grid
TOPICS:
  
  - /camera/depth/color/points [sensor_msgs.msg.PointCloud2]
  - /t265/odom/sample
SERVICES:
  - 
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	autonomous
AUTHOR(S):	Max
CREATION:	17/02/2022
EDITED:		17/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - a lot 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from mapping.mapper import Mapper
from mapping.grid_2d import Grid2D
import time
import numpy as np
import math_utils.transform as transform
from config.runtime_params import max_fov_angle, max_point_depth, max_safe_obstacle
from height_mapper import get_obstacles
from scipy.signal import convolve2d

class HeightMapper(Mapper):
    def __init__(self, length=20, width=20, height=5, resolution=0.1, detection_resolution=0.025, planner=None, _vis=True):

        # init node with node name points
        super().__init__(length, width, height, resolution, planner, _vis)
        self.planning_resolution = resolution
        self.detection_resolution = detection_resolution
        self.resolution_ratio = int(self.planning_resolution / self.detection_resolution)
        self.detection_length = int(np.ceil((max_point_depth / self.detection_resolution) / self.resolution_ratio) * self.resolution_ratio) 
        self.detection_width = int(np.ceil(2 * self.detection_map_length * np.tan(max_fov_angle)))

    def initialise_map(self):
        self._map = Grid2D(self.length, self.width, self.resolution) 

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
        indexes[:, 1] += (np.ceil(self.detection_width/2)).astype(int)
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
        kernel = np.ones((self.resolution_ratio, self.resolution_ratio))
        downscaled = convolve2d(obstacles, kernel, mode='valid')[::self.resolution_ratio, ::self.resolution_ratio].astype(float)
        downscaled /= max_safe_obstacle # ignoring minor hills
        downscaled[downscaled > 1.0] = 1.0 # all points greater than one are just set to 1
        return downscaled

    def arrange_obstacles(self, obstacles):
        """
        Turns a 1d numpy array of obstacle values into a list of coordinates and their
        values. We then cut all points which aren't in the segment within the fov of
        the rover. Finally, transforms the coordinates to fit with the global map.
        :param: obstacles - 1-dimensional array of obstacles in the map
        """
        obs_as_points = np.array([[x, y, val] for (x, y), val in np.ndenumerate(obstacles) \
                if np.abs(np.arctan2(y - len(obstacles[0])/2, x)) < max_fov_angle])
        obs_as_points[:, 1] -= int(np.ceil(self.detection_width/(2 * self.resolution_ratio)))
        obstacles = transform.transform_yaw(self.msg, obs_as_points)
        obstacles[:, 2] *= 100
        return np.round(obstacles).astype(int)

    def get_2d_map(self):
        """
        Returns the 2d version of the map according to this Mapper's mapping policy.
        Default Mapper class simply adds slices above a pre-defined z coordinate.
        """
        return self._map.map.astype(float) / 100

    def handle_pc(self, pts):
        """
        Uses height mapping to identify obstacles in 3d point cloud and generate a 2d
        Occupancy grid for planning on
        :param pts: list of points in meters coordinates relative to the tracking camera
        (not transformed).
        """
        # transforming pitch and roll to flatten the map, but no yaw or translation
        no_yaw_pts = transform.transform_points_no_yaw(self.msg, pts)

        filtered_indices = self.filter_points(no_yaw_pts)
        # cpp function finds steep areas in the high resolution map
        obstacles = get_obstacles(filtered_indices, self.detection_length, self.detection_width)

        # downscaling from high_resolution planning map to low_resolution map
        obs = self.downscale_obs(obstacles)

        rotated_obs = self.arrange_obstacles(self.msg, obs)
        self._map.add_obstacles(self.msg, rotated_obs)
        self._map.publish_grid()

