__package__ = "autonomous"
#!/usr/bin/python3
  

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team. Child class of the Mapper 
class that maps the 2d surroundings with both a
height mapper and a plane fitter. Both maps are
then fused to take advantage of the strengths of
both algorithms.
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
CREATION:	25/02/2022
EDITED:		25/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - a lot 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from mapping.height_mapper import HeightMapper
from mapping.plane_mapper import PlaneMapper
from mapping.flat_mapper import FlatMapper
from mapping.grid_2d import Grid2D
import time
import numpy as np
import math_utils.transform as transform
from config.runtime_params import max_fov_angle, max_point_depth, max_safe_obstacle, min_point_density
from height_mapper import get_obstacles as get_height_obstacles
from plane_fitter import get_obstacles as get_plane_obstacles
from scipy.signal import convolve2d


class HeightPlaneMapper(FlatMapper):
    def __init__(self, length=20, width=20, height=5, resolution=0.1, detection_resolution=0.025, planner=None, _vis=True, camera=False):

        # init node with node name points
        super().__init__(length=length, width=width, height=height, resolution=resolution, detection_resolution=detection_resolution, planner=planner, _vis=_vis, camera=camera)
        self.height_mapper = HeightMapper(length=length, width=width, height=height, resolution=resolution, detection_resolution=detection_resolution)
        self.plane_mapper = PlaneMapper(length=length, width=width, height=height, resolution=resolution, detection_resolution=detection_resolution)

    def get_height_obstacles(self, filtered_indices):
        return get_height_obstacles(filtered_indices, self.detection_length, self.detection_width)
        
    def get_plane_obstacles(self, filtered_indices):
        return get_plane_obstacles(filtered_indices, self.detection_length, self.detection_width, self.resolution_ratio)

    def handle_pc(self, pts):
        """
        Uses height mapping to identify obstacles in 3d point cloud and generate a 2d
        Occupancy grid for planning on
        :param pts: list of points in meters coordinates relative to the tracking camera
        (not transformed).
        """
        # If we want the 3d map as well
        super().handle_pc(pts)
        # transforming pitch and roll to flatten the map, but no yaw or translation
        no_yaw_pts = transform.transform_points_no_yaw(self.msg, pts)

        filtered_indices = self.filter_points(no_yaw_pts)

        # cpp functions finds steep areas in the high resolution map
        height_obstacles, min_h_x = self.get_height_obstacles(filtered_indices)
        plane_obstacles, min_p_x = self.get_plane_obstacles(filtered_indices)

        # downscaling from high_resolution planning map to low_resolution map
        plane_obs, min_p_x = self.plane_mapper.downscale_obs(plane_obstacles, min_p_x)
        height_obs, min_h_x = self.height_mapper.downscale_obs(height_obstacles, min_h_x)
        # min_h_x and min_p_x should now be the same or very similar. Average them
        min_x = 0.5 * (min_p_x + min_h_x)
        
        # any sharp drops located in the height mapper are added to the plane mapper
        assert(plane_obs.shape == height_obs.shape)
        plane_obs[height_obs >= 1.0] = 1.0;

        rotated_obs = self.arrange_obstacles(plane_obs, min_x)
        self._map.add_obstacles(self.msg, rotated_obs)

