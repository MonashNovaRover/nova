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

from mapping.flat_mapper import FlatMapper
from mapping.grid_2d import Grid2D
import time
import numpy as np
import math_utils.transform as transform
from config.runtime_params import max_fov_angle, max_point_depth, max_safe_obstacle, min_point_density
from height_mapper import get_obstacles as get_height_obstacles
from scipy.signal import convolve2d

class HeightMapper(FlatMapper):
    def __init__(self, length=20, width=20, height=5, resolution=0.1, detection_resolution=0.025, planner=None, _vis=True, camera=False):

        # init node with node name points
        super().__init__(length=length, width=width, height=height, resolution=resolution, detection_resolution=detection_resolution, planner=planner, _vis=_vis, camera=camera)

    def get_obstacles(self, filtered_indices):
        return get_height_obstacles(filtered_indices, self.detection_length, self.detection_width)
        
    def handle_pc(self, pts):
        """
        Uses height mapping to identify obstacles in 3d point cloud and generate a 2d
        Occupancy grid for planning on
        :param pts: list of points in meters coordinates relative to the tracking camera
        (not transformed).
        """
        # If we want the 3d map as well
        #super().handle_pc(pts)
        # transforming pitch and roll to flatten the map, but no yaw or translation
        self.check_position_in_map()
        no_yaw_pts = transform.transform_points_no_yaw(self.cam_odom, pts)

        filtered_indices = self.filter_points(no_yaw_pts)

        # cpp function finds steep areas in the high resolution map
        obstacles, min_x = self.get_obstacles(filtered_indices)

        # downscaling from high_resolution planning map to low_resolution map
        obs, min_x = self.downscale_obs(obstacles, min_x)

        rotated_obs = self.arrange_obstacles(obs, min_x)
        self._map.add_obstacles(self.cam_odom, self.offset, rotated_obs)

