#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team. 
Mapper class that subscribes to pointclouds and
uses tf2 transforms to place them in the map. 
Calls C++ processing functions to handle the
conversion of pointclouds into occupancy grids.
Publishes the occupancy grid over ros for use by
the path planner and visualisation.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: mapper
TOPICS:
  - /depth_camera/d435_1/cloud [PointCloud2]
  - /autonomous/occupancy_grid [OccupancyGrid]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	autonomous
AUTHOR(S):	Max Tory
CREATION:	28/02/2022
EDITED:		15/05/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - test post-refactor with pointcloud input
 - test mapping 'invalid' occupancy grids in rviz
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from autonomous.mapping.grid_2d import Grid2D
from autonomous.math_utils import transform
from autonomous.cameras.pc_converter import read_points
from autonomous.config.runtime_params import (
    max_fov_horizontal, 
    max_fov_vertical, 
    max_point_depth, 
    max_safe_obstacle, 
    min_point_density, 
    obstacle_halve_value, 
    obstacle_ignore_value, 
    max_safe_inc,
    min_map_update_time
)
from plane_fitter import get_obstacles as get_plane_obstacles
from height_mapper import get_obstacles as get_height_obstacles

# ros imports
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration
from tf2_ros import TransformBroadcaster, StaticTransformBroadcaster, TransformListener, Buffer

# python imports
import time, logging
from typing import Tuple
import numpy as np
from scipy.signal import convolve2d

from nav_msgs.msg import OccupancyGrid, MapMetaData
from geometry_msgs.msg import TransformStamped, Transform
from sensor_msgs.msg import PointCloud2


class Mapper(Node):
    def __init__(self):
        # init node with node name points
        super().__init__('mapper')

        self.get_logger().set_level(logging.INFO)
        # Timer frequencies
        self.param_tf_sub_hz = self.declare_parameter("tf_sub_frequency_hz", 30).value
        self.param_tf_pub_hz = self.declare_parameter("tf_pub_frequency_hz", 30).value
        self.param_map_pub_hz = self.declare_parameter("map_pub_frequency_hz", 3).value

        # Map dimensions and rolling
        self.param_roll_map = self.declare_parameter("roll_map", True).value
        self.param_map_roll_distance = self.declare_parameter("map_roll_dist_m", 5).value
        self.param_map_corners_coords = self.declare_parameter("map_corners_coords", [10, 10, -10, 10, -10, -10, 10, -10]).value
        self.param_map_len_m = self.declare_parameter("map_len_m", 20).value
        self.param_map_width_m = self.declare_parameter("map_width_m", 20).value
        self.param_resolution_m = self.declare_parameter("resolution_m", 0.1).value
        self.param_detection_resolution_m = self.declare_parameter("detection_resolution_m", 0.025).value

        # How to map obstacles
        self.param_do_height_mapping = self.declare_parameter("do_height_mapping", True).value
        self.param_do_plane_mapping = self.declare_parameter("do_plane_mapping", True).value
        self.do_mapping = self.param_do_height_mapping or self.param_do_plane_mapping

        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self, spin_thread=True)
        if self.do_mapping:
            # In case we just want to point and shoot without worrying about obstacles, we should avoid the extra ros overhead of
            # Subscribing to pointclouds
            self.sub_pointcloud = self.create_subscription(PointCloud2, "/depth_camera/d435_1/cloud", self.pointcloud_callback, 10)

        # For publishing the map
        self.pub_occupancy_grid = self.create_publisher(OccupancyGrid, "/autonomous/occupancy_grid", 10)

        self.previous_map_update = time.perf_counter()

        # For moving the map as we navigate
        if not self.param_roll_map:
            self.tf_map_offset = StaticTransformBroadcaster(self)
        else:
            self.tf_map_offset = TransformBroadcaster(self)

        self.local_map_to_base_link: Transform = None
        self.local_map_to_d435: Transform = None
        self.orient_nova_frame_transform : Transform = None

        self.resolution_ratio = int(self.param_resolution_m / self.param_detection_resolution_m)
        self.detection_length = int(
            np.ceil((max_point_depth / self.param_detection_resolution_m) / self.resolution_ratio) * self.resolution_ratio)
        self.detection_width = int(np.ceil(2 * self.detection_length * np.tan(max_fov_horizontal)))

        self.offset = None
        self.map_centre = None
        self.map_rotation = None
        self.initialised = False
        
        # Position of depth camera in local map
        self.initialise_map()

        # Wait for localisation transforms
        self.get_logger().info("Waiting for transform from 'local_map' to 'd435_1'...")
        while not self.tf_buffer.can_transform('d435_1', 'local_map', Time()):
            time.sleep(0.1)
            if self.param_roll_map:
                self.pub_transform()
        self.get_logger().info("Received Transform!")

        self.initialise_transforms()

        if self.param_roll_map:
            self.map_roll_timer = self.create_timer(1, self.check_position_in_map)
            self.pub_transform_timer = self.create_timer(1./self.param_tf_pub_hz, self.pub_transform)
        self.map_transform_timer = self.create_timer(1./self.param_tf_sub_hz, self.update_transforms)
        self.map_pub_timer = self.create_timer(1./self.param_map_pub_hz, self.publish)

    def initialise_map(self):
        if self.param_roll_map:
            self.set_offset(0, 0)
            self.grid_2d = Grid2D(length=self.param_map_len_m, width=self.param_map_width_m, resolution=self.param_resolution_m, with_border=False)
        else:
            self.map_corners = np.array(self.param_map_corners_coords).reshape(-1, 2)
            self.map_centre = np.mean(self.map_corners, axis=0).astype(float)
            length, width, theta = self.get_map_pose()
            self.map_rotation = theta
            self.grid_2d = Grid2D(length=length, width=width, resolution=self.param_resolution_m, with_border=True)
        self.pub_transform()

    def on_initialised(self):
        self.initialised = True

    def get_furthest_point_in_direction(self, pts, direction):
        """
        Take a list of points and a direction vector. Return the point furthest away from the origin in the direction of the 
        direction vector by comparing the magnitude of dot products with the direction vector
        """
        dots = [np.dot(pt, direction) for pt in pts]
        max_dot = max(dots)
        max_index = dots.index(max_dot)
        return pts[max_index]

    def get_map_pose(self) -> Tuple[float, float, float]:
        """
        Use the four corners of the map in self.map_corners to calculate the length and width of the map, as well as its orientation
        Takes dot products with the four diagonal vectors to determine the closest corner to each direction, so that we can provide
        corners in any order
        """
        top_left = self.get_furthest_point_in_direction(self.map_corners, [1, 1])
        bottom_left = self.get_furthest_point_in_direction(self.map_corners, [-1, 1])
        top_right = self.get_furthest_point_in_direction(self.map_corners, [1, -1])

        len = np.linalg.norm(top_left - bottom_left)
        width = np.linalg.norm(top_left - top_right)
        theta = np.arctan2((top_left - bottom_left)[1], (top_left - bottom_left)[0])
        self.get_logger().debug(f"top left: {top_left}")
        self.get_logger().debug(f"bottom left: {bottom_left}")
        self.get_logger().debug(f"top right: {top_right}")

        self.get_logger().debug(f"length: {len}")
        self.get_logger().debug(f"width: {width}")
        self.get_logger().debug(f"theta: {theta}")

        return len, width, theta

    def shift_offset(self, dx, dy):
        if self.offset is None: return None
        self.set_offset(self.offset[0] + dx, self.offset[1] + dy)

    def initialise_transforms(self):
        """
        Set correct initial transform values, awaiting transforms from tf2
        """
        self.local_map_to_d435: Transform = None
        self.local_map_to_base_link: Transform = None
        try:
            self.orient_nova_frame_transform = self.tf_buffer.lookup_transform(
                target_frame="d435_1_forward", 
                source_frame="d435_1",
                time=Time()).transform
        except:
            self.get_logger().error(f"Couldn't get depth camera rotation!")


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
        self.get_logger().debug("transform publish callback called", throttle_duration_sec=1)
        t = TransformStamped()
        t.header.stamp = self.get_clock().now().to_msg()
        t.header.frame_id = 'map'
        t.child_frame_id = 'local_map'

        # For now we assume the map frame never needs to rotate or move in z axis
        if self.param_roll_map:
            t.transform.translation.x = float(self.offset[0])
            t.transform.translation.y = float(self.offset[1])
            t.transform.rotation.w = 1.0
        else:
            t.transform.translation.x, t.transform.translation.y = self.map_centre
            t.transform.rotation.z = np.sin(self.map_rotation / 2)
            t.transform.rotation.w = np.cos(self.map_rotation / 2)

        self.get_logger().debug(f"Publishing local map transform {t}", throttle_duration_sec=1)

        self.tf_map_offset.sendTransform(t)

    def crop_to_fov(self, points):
        """
        crop points to the field of view of the depth camera
        """
        self.get_logger().debug(f"Points before fov crop: {points}, len = {len(points)}")
        points = points[np.abs(np.arctan2(points[:, 1], points[:, 0])) < max_fov_horizontal]
        points = points[np.abs(np.arctan2(points[:, 2], points[:, 0])) < max_fov_vertical]
        self.get_logger().debug(f"Points after fov crop: {points}, len = {len(points)}")
        return points

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
        indexes = np.floor((points/self.param_detection_resolution_m)).astype(int)
        indexes[:, 1] += (np.ceil(self.detection_width/2)).astype(int)
        return indexes

    def filter_points(self, points):
        """
        Discretises point cloud into indices, then filters out indices without
        enough points in them to avoid phantom "floating" points
        """
        points = self.crop_to_fov(points)
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
        x_dist = self.local_map_to_base_link.translation.x
        y_dist = self.local_map_to_base_link.translation.y
        self.get_logger().debug(f"Edge distances: x = {x_dist}, y = {y_dist}")
        self.get_logger().debug(f"local map -> base link = {self.local_map_to_base_link}")
        if abs(x_dist) > self.param_map_roll_distance:
            x_change = self.param_map_roll_distance * np.sign(x_dist)
        if abs(y_dist) > self.param_map_roll_distance:
            y_change = self.param_map_roll_distance * np.sign(y_dist)
        if x_change != 0 or y_change != 0:
            self.grid_2d.roll_map(x_change, y_change)
        self.shift_offset(x_change, y_change)

    def arrange_obstacles(self, obstacles):
        """
        Turns a 1d numpy array of obstacle values into a list of coordinates and their
        values. We then cut all points which aren't in the segment within the fov of
        the rover. Finally, transforms the coordinates to fit with the local map.
        :param: obstacles - 1-dimensional array of obstacles in the map
        """
        obs_as_points = np.array([[x, y, val] for (x, y), val in np.ndenumerate(obstacles) \
                                  if np.abs(np.arctan2(y - len(obstacles[0]) / 2, x)) < max_fov_horizontal])
        obs_as_points[:, 1] -= int(np.ceil(self.detection_width / (2 * self.resolution_ratio)))
        self.get_logger().debug(f"Rotating obstacles in map: {self.local_map_to_d435}", throttle_duration_sec=1)
        obstacles = transform.transform_yaw(self.local_map_to_d435, obs_as_points)
        obstacles[:, 2] *= 100

        # halving non-obstacle values to make us not care so much
        obstacles[obstacles[:, 2] < obstacle_halve_value] /= np.array([1, 1, 2])

        # all points below a certain value get set to the minimum value
        obstacles[obstacles[:, 2] < obstacle_ignore_value, 2] = 5
        return np.round(obstacles).astype(int)

    def update_transforms(self):
        """
        Listen for new tf2 transforms
        """
        try:
            self.local_map_to_d435 = self.tf_buffer.lookup_transform(target_frame='local_map',
                                                                source_frame='d435_1_forward',
                                                                time=Time()).transform
        except Exception as e:
            self.get_logger().debug(f"transform lookup error for d435 transform: {e}", throttle_duration_sec=1)
        try:
            self.local_map_to_base_link = self.tf_buffer.lookup_transform(target_frame='local_map',
                                                                source_frame='base_link',
                                                                time=Time()).transform
        except Exception as e:
            self.get_logger().debug(f"transform lookup error for base_link transform: {e}", throttle_duration_sec=1)

    def update_map(self, pts):
        """
        We only update the map every .5 seconds at most - need to take out magic number
        :param pts: np.array(n, 6) - refers to x,y,z,r,g,b
        """
        # transform the points
        self.get_logger().debug(f"Update map called after {time.perf_counter() - self.previous_map_update} s with {len(pts)} points", throttle_duration_sec=1)
        if time.perf_counter() - self.previous_map_update > min_map_update_time:
            self.get_logger().debug(f"handling pc: {pts}", throttle_duration_sec=1)
            self.handle_pc(pts)

            self.previous_map_update = time.perf_counter()

    def pointcloud_callback(self, msg):
        """
        This is called when the depth camera receives a new set of points via the python api. It is implemented
        as a callback so it can happen in a separate thread.
        It calls a function to extract and filter the points (colors are ignored) and updates the map with points only -
        when using the python API, it should be a points only map.
        """
        if not self.initialised:
            return
        self.get_logger().debug("Received pointcloud", throttle_duration_sec=1)
        self.update_map(np.array(list(read_points(msg, skip_nans=True))))

    def get_2d_map(self):
        """
        Returns the 2d version of the map according to this Mapper's mapping policy.
        Default Mapper class simply adds slices above a pre-defined z coordinate.
        """
        return self.grid_2d.map.astype(float) / 100

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
        if len(pts) < 10:
            return
        self.get_logger().debug(f"Transforming point cloud by transform: {self.orient_nova_frame_transform}", throttle_duration_sec=1)
        # transform to nova coordinates
        frame_transformed_points = transform.transform_points(self.orient_nova_frame_transform, pts)
        self.get_logger().debug(f"Transforming point cloud by transform: {self.local_map_to_d435}", throttle_duration_sec=1)
        no_yaw_pts = transform.transform_points_no_yaw(self.local_map_to_d435, frame_transformed_points)

        filtered_indices = self.filter_points(no_yaw_pts)

        height_obstacles, plane_obstacles = None, None
        # cpp functions finds steep areas in the high resolution map
        if self.param_do_plane_mapping:
            plane_obstacles, min_p_x = get_plane_obstacles(filtered_indices)
            scaled_safe_inc = max_safe_inc * 255 / 90
            plane_obs = plane_obstacles.astype(float) / scaled_safe_inc
            plane_obs[plane_obs > 1.0] = 1.0
            plane_obs = plane_obs[min_p_x:, :]
        if self.param_do_height_mapping:
            height_obstacles, min_h_x = get_height_obstacles(filtered_indices)
            height_obs, min_h_x = self.downscale_obs(height_obstacles, min_h_x)
            height_obs = height_obs[min_h_x:, :]
        
        if self.param_do_plane_mapping:
            obstacles = plane_obs
            if self.param_do_height_mapping:
                # any sharp drops located in the height mapper are added to the plane mapper
                obstacles[height_obs >= 1.0] = 1.1
        else:
            obstacles = height_obs
        
        rotated_obs = self.arrange_obstacles(obstacles)
        self.grid_2d.add_obstacles(self.local_map_to_d435, rotated_obs)

    def publish(self):
        """
        Publish the 2d map over ros to be viewed in RVIZ
        """
        self.get_logger().debug("Publishing grid...")

        meta_data = MapMetaData()
        meta_data.resolution = self.grid_2d.resolution
        meta_data.width = int(self.grid_2d.outer_width / self.grid_2d.resolution)
        meta_data.height = int(self.grid_2d.outer_length / self.grid_2d.resolution)

        meta_data.map_load_time = self.get_clock().now().to_msg()
        meta_data.origin.position.x = -self.grid_2d.outer_length / 2
        meta_data.origin.position.y = -self.grid_2d.outer_width / 2
        meta_data.origin.orientation.w = 1.0

        grid = OccupancyGrid()
        grid.header.stamp = self.get_clock().now().to_msg()
        grid.header.frame_id = 'local_map'
        grid.info = meta_data
        grid.data = self.grid_2d.as_bytes()

        self.get_logger().debug(f"Publishing occupancy grid: {grid}")

        self.pub_occupancy_grid.publish(grid)



def main():
    rclpy.init()
    mapper = Mapper()
    rclpy.spin(mapper)
    mapper.destroy_node()
    rclpy.shutdown()


if __name__=='__main__':
    main()