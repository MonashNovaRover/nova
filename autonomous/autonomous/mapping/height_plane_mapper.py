#!/usr/bin/python3
__package__ = "autonomous"


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
EDITED:		31/12/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
  - Correctly transform points
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from autonomous.mapping.python_height_mapper import HeightMapper
from autonomous.mapping.python_plane_mapper import PlaneMapper
from autonomous.mapping.flat_mapper import FlatMapper
import autonomous.math_utils.transform as transform
import logging


class HeightPlaneMapper(FlatMapper):
    def __init__(self, length=20, width=20, height=5, resolution=0.1, detection_resolution=0.025, planner=None, camera=False, name="height_plane_mapper"):

        # init node with node name points
        super().__init__(length=length, width=width, height=height, resolution=resolution, detection_resolution=detection_resolution, planner=planner, camera=camera, name=name)
        self.get_logger().set_level(logging.DEBUG)
        self.height_mapper = HeightMapper(length=length, width=width, height=height, resolution=resolution, detection_resolution=detection_resolution)
        self.plane_mapper = PlaneMapper(length=length, width=width, height=height, resolution=resolution, detection_resolution=detection_resolution)

    def handle_pc(self, pts):
        """
        Uses height mapping to identify obstacles in 3d point cloud and generate a 2d
        Occupancy grid for planning on
        :param pts: list of points in meters coordinates relative to the tracking camera
        (not transformed).
        """
        # If we want the 3d map as well
        # super().handle_pc(pts)
        # transforming pitch and roll to flatten the map, but no yaw or translation
        if self.local_map_to_d435 is None: 
            self.get_logger().warn("No transform to d435 frame!")
            return
        if self.orient_nova_frame_transform is None: 
            self.get_logger().warn("No transform to forward-facing frame!")
            return
        self.get_logger().debug(f"Transforming point cloud by transform: {self.orient_nova_frame_transform}")
        # transform to nova coordinates
        frame_transformed_points = transform.transform_points(self.orient_nova_frame_transform, pts)
        self.get_logger().debug(f"Transforming point cloud by transform: {self.local_map_to_d435}")
        no_yaw_pts = transform.transform_points_no_yaw(self.local_map_to_d435, frame_transformed_points)

        filtered_indices = self.filter_points(no_yaw_pts)

        # cpp functions finds steep areas in the high resolution map
        height_obstacles, min_h_x = self.height_mapper.get_obstacles(filtered_indices)
        plane_obstacles, min_p_x = self.plane_mapper.get_obstacles(filtered_indices)

        # downscaling from high_resolution planning map to low_resolution map
        plane_obs, min_p_x = self.plane_mapper.downscale_obs(plane_obstacles, min_p_x)
        height_obs, min_h_x = self.height_mapper.downscale_obs(height_obstacles, min_h_x)
        # min_h_x and min_p_x should now be the same or very similar. Average them
        min_x = 0.5 * (min_p_x + min_h_x)
        
        # any sharp drops located in the height mapper are added to the plane mapper
        assert(plane_obs.shape == height_obs.shape)
        plane_obs[height_obs >= 1.0] = 1.1
        
        rotated_obs = self.arrange_obstacles(plane_obs, min_x)
        self._map.add_obstacles(self.local_map_to_d435, rotated_obs)

        self.get_logger().debug("publishing map")
        self.publish()

