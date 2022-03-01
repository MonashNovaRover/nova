__package__ = "autonomous"
#!/usr/bin/python3
  

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team. Abstract child class of the 
Mapper class for all mappers that map the 3d
surrounds in a 2d map. These mappers all require
common methods, such as separate yaw/pitch+roll
transformations, publishing, and getting
indices.  
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
CREATION:	28/02/2022
EDITED:		28/02/2022
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
from config.runtime_params import max_fov_angle, max_point_depth, max_safe_obstacle, min_point_density
from scipy.signal import convolve2d

class FlatMapper(Mapper):
    def __init__(self, length=20, width=20, height=5, resolution=0.1, detection_resolution=0.025, planner=None, _vis=True, camera=False):

        # init node with node name points
        super().__init__(length=length, width=width, height=height, resolution=resolution, planner=planner, _vis=_vis, camera=camera)
        self.planning_resolution = resolution
        self.detection_resolution = detection_resolution
        self.resolution_ratio = int(self.planning_resolution / self.detection_resolution)
        self.detection_length = int(np.ceil((max_point_depth / self.detection_resolution) / self.resolution_ratio) * self.resolution_ratio) 
        self.detection_width = int(np.ceil(2 * self.detection_length * np.tan(max_fov_angle)))
        self.initialise_map()

    def initialise_map(self):
        self._map = Grid2D(self.length, self.width, self.planning_resolution) 

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
        points = points[abs(points[:,2]) < 1.5]
        indexes = self.get_detection_map_indexes(points)
        indexes, counts = np.unique(indexes, return_counts=True, axis=0)
        counts = (counts // min_point_density).astype(bool) # filtering out voxels without many points in them
        return indexes[counts]

    def downscale_obs(self, obstacles, min_x):
        """
        Uses convolution with a kernel of ones to add up the values in sections
        of the grid so that we can down-size the resolution.
        """
        kernel = np.ones((self.resolution_ratio, self.resolution_ratio))
        downscaled = convolve2d(obstacles, kernel, mode='valid')[::self.resolution_ratio, ::self.resolution_ratio].astype(float)
        downscaled /= max_safe_obstacle # ignoring minor hills
        downscaled[downscaled > 1.0] = 1.0 # all points greater than one are just set to 1
        min_x /= self.resolution_ratio
        return downscaled, int(min_x)

    def arrange_obstacles(self, obstacles, min_x):
        """
        Turns a 1d numpy array of obstacle values into a list of coordinates and their
        values. We then cut all points which aren't in the segment within the fov of
        the rover. Finally, transforms the coordinates to fit with the global map.
        :param: obstacles - 1-dimensional array of obstacles in the map
        """
        obs_as_points = np.array([[x, y, val] for (x, y), val in np.ndenumerate(obstacles) \
                if np.abs(np.arctan2(y - len(obstacles[0])/2, x)) < max_fov_angle and x > min_x])
        obs_as_points[:, 1] -= int(np.ceil(self.detection_width/(2 * self.resolution_ratio)))
        obstacles = transform.transform_yaw(self.msg, obs_as_points)
        obstacles[:, 2] *= 100
        obstacles[obstacles[:, 2] < 100, 2] = 5
        return np.round(obstacles).astype(int)

    def get_2d_map(self):
        """
        Returns the 2d version of the map according to this Mapper's mapping policy.
        Default Mapper class simply adds slices above a pre-defined z coordinate.
        """
        return self._map.map.astype(float) / 100

    def publish(self):
        """
        Publish the 2d map over ros to be viewed in RVIZ
        """
        self._map.publish_grid()
        super().publish()
