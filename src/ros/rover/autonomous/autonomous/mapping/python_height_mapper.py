#!/usr/bin/python3
__package__ = "autonomous"


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
  - /t25/odom/sample
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

from autonomous.mapping.flat_mapper import FlatMapper
import autonomous.math_utils.transform as transform
import logging
from height_mapper import get_obstacles as get_height_obstacles


class HeightMapper(FlatMapper):
    def __init__(self, length=20, width=20, height=5, resolution=0.1, detection_resolution=0.025, planner=None,
                 camera=False, name="height_mapper"):

        # init node with node name points
        super().__init__(length=length, width=width, height=height, resolution=resolution,
                         detection_resolution=detection_resolution, planner=planner, camera=camera, name=name)
        self.get_logger().set_level(logging.INFO)
        self.on_initialised()

    def get_obstacles(self, filtered_indices):
        return get_height_obstacles(filtered_indices, self.detection_length, self.detection_width)
        
    def handle_pc(self, pts):
        """
        Uses height mapping to identify obstacles in 3d point cloud and generate a 2d
        Occupancy grid for planning on
        :param pts: list of points in meters coordinates relative to the tracking camera
        (not transformed).
        """
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

        # cpp function finds steep areas in the high resolution map
        obstacles, min_x = self.get_obstacles(filtered_indices)

        # downscaling from high_resolution planning map to low_resolution map
        obs, min_x = self.downscale_obs(obstacles, min_x)

        rotated_obs = self.arrange_obstacles(obs, min_x)
        self._map.add_obstacles(self.local_map_to_d435, rotated_obs)

        self.get_logger().warn("publishing map")
        self.publish()
