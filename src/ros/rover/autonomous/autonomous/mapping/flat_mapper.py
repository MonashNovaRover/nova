__package__ = "autonomous"
# !/usr/bin/python3


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
SERVICES: None
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

from autonomous.mapping.mapper import Mapper
from autonomous.mapping.grid_2d import Grid2D
import numpy as np
import autonomous.math_utils.transform as transform
from autonomous.config.runtime_params import max_fov_angle, max_point_depth, max_safe_obstacle, min_point_density, \
    obstacle_halve_value, obstacle_ignore_value
from scipy.signal import convolve2d
from rclpy.time import Time
from rclpy.duration import Duration
import time, math, logging

from geometry_msgs.msg import TransformStamped, Transform
from tf2_ros import TransformBroadcaster


class FlatMapper(Mapper):
    def __init__(
            self,
            length=20,
            width=20,
            height=5,
            resolution=0.1,
            detection_resolution=0.025,
            planner=None,
            camera=False,
            name='flat_mapper',
    ):

        # init node with node name points
        super().__init__(
            length=length,
            width=width,
            height=height,
            resolution=resolution,
            planner=planner,
            camera=camera,
        )

        self.get_logger().set_level(logging.DEBUG)
        self.param_tf_sub_hz = self.declare_parameter("tf_sub_frequency_hz", 30).value
        self.param_tf_pub_hz = self.declare_parameter("tf_pub_frequency_hz", 30).value
        self.param_roll_map = self.declare_parameter("roll_map", False).value
        self.param_map_edge_distance = self.declare_parameter("map_edge_dist_m", 3).value
        # How far to roll the map when we approach the edge
        self.param_map_roll_distance = self.declare_parameter("map_roll_dist_m", 5).value   
        # For moving the map as we navigate
        self.tf_map_offset = TransformBroadcaster(self)

        self.planning_resolution = resolution     # resolution of occupancy grid for path planning
        self.detection_resolution = detection_resolution    # resolution of obstacle deteciton grid
        self.resolution_ratio = int(self.planning_resolution / self.detection_resolution)
        self.detection_length = int(
            np.ceil((max_point_depth / self.detection_resolution) / self.resolution_ratio) * self.resolution_ratio)
        self.detection_width = int(np.ceil(2 * self.detection_length * np.tan(max_fov_angle)))

        # Position of depth camera in local map
        self.initialise_map()
        self.initialise_transforms()

        if self.param_roll_map:
            self.map_roll_timer = self.create_timer(1, self.check_position_in_map)
        self.pub_transform_timer = self.create_timer(1./self.param_tf_pub_hz, self.pub_transform)
        self.map_transform_timer = self.create_timer(1./self.param_tf_sub_hz, self.update_transforms)

    def initialise_map(self):
        self._map = Grid2D(self.length, self.width, self.planning_resolution)
        self.set_offset(0, 0)

    def shift_offset(self, dx, dy):
        if self.offset is None: return None
        self.set_offset(self.offset[0] + dx, self.offset[1] + dy)

    def initialise_transforms(self):
        """
        Set correct initial transform values, awaiting transforms from tf2
        """
        self.local_map_to_d435: Transform = None
        self.local_map_to_base_link: Transform = None

        while self.local_map_to_d435 is None or\
                self.local_map_to_base_link is None:
            self.update_transforms()

    def set_offset(self, x, y):
        """
        Save new map offset x and y coordinates. These are the offset of the map frame from the 'world' frame.
        Broadcasts the new transform to tf2
        """
        self.offset = [x, y]

    def pub_transform(self):
        """
        regularly publish transform from map to local map
        """
        t = TransformStamped()
        t.header.stamp = self.get_clock().now().to_msg()
        t.header.frame_id = 'map'
        t.child_frame_id = 'local_map'

        # For now we assume the map frame never needs to rotate or move in z axis
        t.transform.translation.x = float(self.offset(0))
        t.transform.translation.y = float(self.offset(1))

        self.tf_map_offset.sendTransform(t)

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
        if len(points) == 0:
            return []
        indexes = np.floor((points/self.detection_resolution)).astype(int)
        indexes[:, 1] += (np.ceil(self.detection_width/2)).astype(int)
        return indexes

    def filter_points(self, points):
        """
        Discretises point cloud into indices, then filters out indices without
        enough points in them to avoid phantom "floating" points
        """
        if len(points) == 0:
            return points
        indexes = self.get_detection_map_indexes(points)
        indexes, counts = np.unique(indexes, return_counts=True, axis=0)
        counts = (counts // min_point_density).astype(bool)  # filtering out voxels without many points in them
        return indexes[counts]

    def downscale_obs(self, obstacles, min_x):
        """
        Uses convolution with a kernel of ones to add up the values in sections
        of the grid so that we can down-size the resolution.
        """
        kernel = np.ones((self.resolution_ratio, self.resolution_ratio))
        downscaled = convolve2d(obstacles, kernel, mode='valid')\
            [::self.resolution_ratio, ::self.resolution_ratio].astype(float)
        downscaled /= max_safe_obstacle  # ignoring minor hills
        downscaled[downscaled > 1.0] = 1.0  # all points greater than one are just set to 1
        min_x /= self.resolution_ratio
        return downscaled, int(min_x)

    def check_position_in_map(self):
        """
        If we're near the edge of the map, roll the map in a given direction.
        Only called if param_roll_map is true
        """
        x_change, y_change = 0, 0
        x_edge_dist = (self._map.length / 2 - abs(self.local_map_to_base_link.translation.x)) *\
            np.sign(self.local_map_to_base_link.translation.x)
        y_edge_dist = (self._map.width / 2 - abs(self.local_map_to_base_link.translation.y)) *\
            np.sign(self.local_map_to_base_link.translation.y)
        if x_edge_dist > -self.param_map_edge_distance:
            x_change = -self.param_map_roll_distance
        elif x_edge_dist < self.param_map_edge_distance:
            x_change = self.param_map_roll_distance
        if y_edge_dist > -self.param_map_edge_distance:
            y_change = -self.param_map_roll_distance
        elif y_edge_dist < self.param_map_edge_distance:
            y_change = self.param_map_roll_distance
        if x_change != 0 or y_change != 0:
            self._map.roll_map(x_change, y_change)
        self.shift_offset(x_change, y_change)

    def update_transforms(self):
        """
        Listen for new tf2 transforms
        """
        try:
            self.local_map_to_d435 = self.tf_buffer.lookup_transform(target_frame='local_map',
                                                                source_frame='d435_1',
                                                                time=Time()).transform
        except Exception as e:
            self.get_logger().debug(f"transform lookup error for d435 transform: {e}")
        try:
            self.local_map_to_base_link = self.tf_buffer.lookup_transform(target_frame='local_map',
                                                                source_frame='base_link',
                                                                time=Time()).transform
        except Exception as e:
            self.get_logger().debug(f"transform lookup error for base_link transform: {e}")

    def arrange_obstacles(self, obstacles, min_x):
        """
        Turns a 1d numpy array of obstacle values into a list of coordinates and their
        values. We then cut all points which aren't in the segment within the fov of
        the rover. Finally, transforms the coordinates to fit with the local map.
        :param: obstacles - 1-dimensional array of obstacles in the map
        """
        obs_as_points = np.array([[x, y, val] for (x, y), val in np.ndenumerate(obstacles) \
                                  if np.abs(np.arctan2(y - len(obstacles[0]) / 2, x)) < max_fov_angle])
        obs_as_points[:, 1] -= int(np.ceil(self.detection_width / (2 * self.resolution_ratio)))
        self.get_logger().debug(f"Rotating obstacles in map: {self.local_map_to_d435}")
        obstacles = transform.transform_yaw(self.local_map_to_d435, obs_as_points)
        obstacles[:, 2] *= 100

        # halving non-obstacle values to make us not care so much
        obstacles[obstacles[:, 2] < obstacle_halve_value] /= np.array([1, 1, 2])

        # all points below a certain value get set to the minimum value
        obstacles[obstacles[:, 2] < obstacle_ignore_value, 2] = 5
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
