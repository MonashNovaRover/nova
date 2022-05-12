#!/usr/bin/python3

__package__ = "autonomous"

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team. Base Mapper class that
maps the 2d surroundings by simply extracting
layers from the 3d map. Extended by other Mappers
with more evolved obstacle detection algorithms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: points_grid
TOPICS:
  - Subscriber: /camera/depth/color/points [sensor_msgs.msg.PointCloud2]
  - Subscriber: /t265/odom/sample [nav_msgs.msg.Odometry]

SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	autonomous
AUTHOR(S):	Lucas, Kelly, Kelvin, Amesh, Liam, Max
CREATION:	27/09/2021
EDITED:		17/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - a lot 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2
import vis.pc_converter as pc2
import math_utils.transform as transform
from sensor_msgs.msg import PointField
from nav_msgs.msg import Odometry
from mapping.grid_3d import Grid3D
import numpy as np
import vis.pc_pub as pc_pub
import time
from cameras.depth_camera import DepthCamera
from config.ros_config import camera_pose_topic, depth_topic
from config.runtime_params import max_point_depth, max_fov_angle, depth_mode, skip_pts, slice_height, planning_rate


class Mapper(Node):
    def __init__(self, length=20, width=20, height=5, resolution=0.1, planner=None, camera=False, _vis=True):
        super().__init__('points_grid')
        self.subscriber_tracking = self.create_subscription(Odometry, camera_pose_topic, self.pose_callback, 100)
        self.planner = planner
        self.vis = _vis

        self.length = length
        self.width = width
        self.height = height
        self.resolution = resolution

        self.previous_plan = time.perf_counter()
        self.previous_map_update = time.perf_counter()

        self.last_cam_odom = None
        self.cam_odom = None

        if self.vis:
            self.pc_pub = pc_pub.PCPub("map_cloud")
        else:
            self.pc_pub = None

        if depth_mode == "ros":
            self.subscriber_points = self.create_subscription(PointCloud2, depth_topic, self.ros_points_callback, 10)
            self.has_color = True

        else: 
            if camera:
                self.camera = DepthCamera(self.python_callback)
                # starts a separate thread which will get depth frames and update mapper
                self.camera.start()
            self.has_color = False

        # initialise map 3d
        self._map3d = None
        self.initialise_map3d()

    def initialise_map3d(self):
        self._map3d = Grid3D(self.length, self.width, self.height, self.resolution, has_color=self.has_color)

    def check_position_in_map(self):
        pass
        
    def extract_layer(self, height_m):
        return self._map3d.extract_z(height_m)

    def get_points_and_colors(self, msg):
        """
        Callback used to get points and colors with type np.array from PointCloud2 data from a ros publisher.
        Also transforms into the Nova left handed coordinate system.
        :param msg: PointCloud2
        :return: (n, 6) np.array
        """

        # we need to re-set the field names to extract the unsigned ints from the msg type (one for r, g, b)
        msg.fields = msg.fields[0:3]
        msg.fields.append(PointField(name="r", offset=16, datatype=2, count=1))
        msg.fields.append(PointField(name="g", offset=17, datatype=2, count=1))
        msg.fields.append(PointField(name="b", offset=18, datatype=2, count=1))

        # 1. Parse raw point-cloud data into array of (x, y, z) tuples
        arr = list(pc2.read_points(msg, field_names=("x", "y", "z", "r", "g", "b"), skip_nans=True))

        # 2. Wrap the point-cloud array in a numpy array
        np_arr = np.array(arr)
        
        # 3. Split into points (x, y, z) and colors (r, g, b) 
        pts = np_arr[:, 0:3]
        colors = np_arr[:, 3:6] / 255.0

        # 4. Swap red and blue (for some reason it's not stored how it should be)
        colors = colors[:, [0, 1, 2]]

        # 5. converting from (x=right, y=down, z=forward) -> (x=forward, y=right, z=up)
        pts = self.convert_pts_to_tracking(pts)

        # 6. prune points
        pts, colors = self.prune_point_cloud(pts, colors=colors)

        # put it all in the one array of shape (n, 6)
        points = np.concatenate((pts, colors), axis=1)

        return points

    @staticmethod
    def prune_point_cloud(pts, colors=None):
        """
        :param pts: np.array with shape (n, 3)
        :param colors: np.array with shape (n, 3)
        :return: np.array with shape (n, 3) or two such arrays as a tuple
        """
        # 1. only taking every 10th value (cos 2 much data)
        pts = pts[::skip_pts]
        if colors:
            colors = colors[::skip_pts]

        # 2. Pruning out points which are either beyond the max dist, or are outside the max angle
        indexes = (Mapper.row_norm(pts) < max_point_depth) & (abs(np.arctan(pts[:, 1] / pts[:, 0])) < max_fov_angle) \
                  & (abs(np.arctan(pts[:, 2] / pts[:, 0])) < max_fov_angle)

        if colors:
            return pts[indexes], colors
        return pts[indexes]

    @staticmethod
    def convert_pts_to_tracking(pts):
        """
        Converts a numpy array of points from (x=right, y=down, z=forward) coordinates to (x=forward, y=right, z=up)
        :param pts: np.array with shape (n, 3), corresponding to an array of [x, y, z] coordinates
        :return: np.array with shape (n, 3)
        """
        pts = pts[:, [2, 0, 1]]
        pts[:, 2] = -pts[:, 2]
        pts[:, 1] = -pts[:, 1]
        return pts

    @staticmethod
    def row_norm(pts):
        """
        Efficient numpy way of doing euclidean distance over each row 
        (just takes all the values in the last index, which will be 3 for our purposes, and calculates the length)
        :param pts: (n, 3) array of points
        :return: what we need to
        """
        return np.sum(np.abs(pts) ** 2, axis=-1) ** (1.0 / 2)

    def handle_pc(self, pts):
        """
        Dictates what the mapper class does to map a new point cloud. Overridden by child classes
        with different mapping implementations
        :param pts: list of points in meters coordinates relative to the tracking camera (not
        transformed).
        """
        # transforming to the global frame
        full_transform_pts = transform.transform_points(self.cam_odom, pts)
        self._map3d.add_pc_points_only(full_transform_pts)

    def publish(self):
        """
        Publishes the map to ros for RVIZ to visualise
        """
        if self.vis:
            pts = self._map3d.get_as_pc()
            max_z = 10
            # setting colors proportional to the height of points - hopefully looks cool!
            colors = np.array([(abs(pts[:, 2]) + 1 / max_z) * 250.0 % 250, np.full(len(pts), 0),
                               abs(max_z - abs(pts[:, 2]) - 1) * 250 % 250]).transpose()
            self.pc_pub.pub_pts_colors(pts, colors.astype(int))

    def get_2d_map(self):
        """
        Returns the 2d version of the map according to this Mapper's mapping policy.
        Default Mapper class simply adds slices above a pre-defined z coordinate.
        """
        layer = self.extract_layer(slice_height)
        return layer.squeeze()

    def update_map3d_pts_only(self, pts):
        """
        :param pts: np.array(n, 6) - refers to x,y,z,r,g,b
        """

        if pts.shape[0] < 10:
            # Not enough points in the point cloud
            return

        # transform the points
        if self.cam_odom and time.perf_counter() - self.previous_map_update > 0.5:
            self.handle_pc(pts)
            self.previous_map_update = time.perf_counter()

        self.publish()

        if time.perf_counter() - self.previous_plan > 1:
            if self.planner:
                # OLD WAY - MAP LAYES
                self.planner.get_path(self.get_2d_map())
                self.previous_plan = time.perf_counter()

    def get_pts(self, pts):
        return self.prune_point_cloud(self.convert_pts_to_tracking(pts))

    def python_callback(self, pts):
        """
        This is called when the depth camera receives a new set of points via the python api. It is implemented
        as a callback so it can happen in a separate thread.
        It calls a function to extract and filter the points (colors are ignored) and updates the map with points only -
        when using the python API, it should be a points only map.
        """
        self.cam_odom = self.last_cam_odom
        if self.cam_odom is not None:
            self.update_map3d_pts_only(self.get_pts(pts))

    def ros_points_callback(self, msg):
        self.cam_odom = self.last_cam_odom
        pts, colors = self.get_points_and_colors(msg)
        self._map3d.add_pc(pts, colors)
        # every 2 seconds we run planning
        if time.perf_counter() - self.previous_plan > planning_rate:
            if self.planner:
                self.previous_plan = time.perf_counter()
                self.planner.get_path(self.extract_layer(2.8))

    def publish_vis_dense(self, extra_pts=1):
        """
        This was an experimental way to increase the density (and thus the aesthetics)
        of visualised point-clouds, but it's probably a bit pointless <hehe>
        Do not use - very inefficient
        """
        pts, colors = self._map3d.get_as_pc()
        colors = colors + [254, 254, 254]
        self.get_logger().info("publishing dense point cloud")
        pts_dense = pts[:]
        colors_dense = colors[:]
        self.pc_pub.pub_pts_colors(pts, colors)
        min_z = np.min(pts[:, 2])
        max_z = np.max(pts[:, 2])
        colors[:, 2] = 254 * abs(pts[:, 2]) / max_z
        colors[:, 0] = 200 * (max_z - abs(pts[:, 2])) / max_z
        colors[:, 1] = 100 * (max_z - abs(pts[:, 2])) / max_z
        for x in range(extra_pts + 1):
            x_shift = [(self.resolution / (extra_pts + 1)) * x, 0, 0]
            for y in range(extra_pts + 1):
                y_shift = [0, (self.resolution / (extra_pts + 1)) * y, 0]
                for z in range(extra_pts + 1):
                    z_shift = [0, 0, (self.resolution / (extra_pts + 1)) * z]
                    pts_dense = np.concatenate((pts_dense, pts + x_shift + y_shift + z_shift))
                    colors_dense = np.concatenate((colors_dense, colors))
        self.pc_pub.pub_pts_colors(pts_dense, colors_dense)

    def pose_callback(self, msg):
        self.last_cam_odom = msg


def position_callback(msg):
    """
    Parses positional data, calculates the average value and publishes
    it to the topic /obstacle_proximity.
    """
    pass


def main(args=None):
    rclpy.init(args=args)
    # reset_cameras.reset_cameras()
    subscriber = Mapper()
    rclpy.spin(subscriber)
    subscriber.destroy_node()
    rclpy.shutdown()


def vis():
    """
    Example function loads a pre existing map into voxel memory and visualises it as a point-cloud
    """
    rclpy.init()
    m = Mapper(length=10, width=10, height=5, resolution=.2)
    m._map3d.map = np.load("resources/environment.npy")
    m.publish_vis_dense(extra_pts=2)


if __name__ == '__main__':
    main()
