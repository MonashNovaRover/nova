__package__ = "autonomous"
#!/usr/bin/python3
  

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team. Child class of the Mapper 
class that maps the 2d surroundings by fitting 
planes to a max-height map over the most recent 
version of the 3d point cloud sent by the Rover. 
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
CREATION:	22/02/2022
EDITED:		28/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - a lot 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from mapping.height_mapper import FlatMapper
from mapping.grid_2d import Grid2D
import time
import numpy as np
import math_utils.transform as transform
from config.runtime_params import max_fov_angle, max_point_depth, max_safe_inc
from plane_fitter import get_obstacles as get_plane_obstacles

class PlaneMapper(FlatMapper):
    def __init__(self, length=20, width=20, height=5, resolution=0.1, detection_resolution=0.025, planner=None, _vis=True, camera=False):

        # init node with node name points
        super().__init__(length=length, width=width, height=height, resolution=resolution, detection_resolution=detection_resolution, planner=planner, _vis=_vis, camera=camera)

    def get_obstacles(self, filtered_indices):
        return get_plane_obstacles(filtered_indices, self.detection_length, self.detection_width, self.resolution_ratio)

    def downscale_obs(self, obstacles, min_x):
        """
        plane_fitter obstacles are already scaled to the planninng resolution,
        so we simply interpret the incs (scaled to 255 by the plane fitter) as
        obstacles or safe
        """
        scaled_safe_inc = max_safe_inc * 255 / 90
        downscaled = obstacles.astype(float) / scaled_safe_inc
        downscaled[downscaled > 1.0] = 1.0
        return downscaled, min_x

    def handle_pc(self, pts):
        # If we want the 3d map as well
        super().handle_pc(pts)
        #transforming pitch and roll to # transforming pitch and roll to flatten the map, but no yaw or translation
        no_yaw_pts = transform.transform_points_no_yaw(self.msg, pts)

        filtered_indices = self.filter_points(no_yaw_pts)

        # cpp function finds jumps in the high-resolution map
        obstacles, min_x = self.get_obstacles(filtered_indices)
        
        # scaling to the map we navigate on
        downscaled_obs, min_x = self.downscale_obs(obstacles, min_x)

        rotated_obs = self.arrange_obstacles(downscaled_obs, min_x)
        self._map.add_obstacles(self.msg, rotated_obs)
        

