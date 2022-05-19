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
import vis.pc_converter as pc2
from sensor_msgs.msg import PointField
from nav_msgs.msg import Odometry
import numpy as np
import time
from cameras.depth_camera import DepthCamera
from config.ros_config import camera_pose_topic
from config.runtime_params import max_point_depth, max_fov_angle, skip_pts, min_map_update_time


class Mapper(Node):
    def __init__(self, length=20, width=20, height=5, planner=None, resolution=0.1, camera=False):
        super().__init__('mapper')
        self.subscriber_tracking = self.create_subscription(Odometry, camera_pose_topic, self.pose_callback, 100)

        self.length = length
        self.width = width
        self.height = height
        self.resolution = resolution

        self.planner = planner

        self.previous_plan = time.perf_counter()
        self.previous_map_update = time.perf_counter()

        self.last_cam_odom = None
        self.cam_odom = None

        # if camera is true we create one, start it, and add a callback. Depth camera runs in a separate thread
        if camera:
            self.camera = DepthCamera(self.python_callback)
            self.camera.start()
        self.has_color = False

    def check_position_in_map(self):
        """
        Abstract method for use in inherited classes.
        """
        pass

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

        # 2. Wrap the point-cloud array in a numpy array np_arr = np.array(arr) 
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
        Abstract method
        """
        # transforming to the global frame
        raise NotImplemented("handle_pc must be implemented in a sub class")

    def get_2d_map(self):
        """
        Abstract method
        """
        raise NotImplemented("get_2d_map must be implemented in a sub class")

    def update_map(self, pts):
        """
        We only update the map every .5 seconds at most - need to take out magic number
        :param pts: np.array(n, 6) - refers to x,y,z,r,g,b
        """
        # transform the points
        if self.cam_odom and time.perf_counter() - self.previous_map_update > min_map_update_time:
            self.handle_pc(pts)

            self.previous_map_update = time.perf_counter()

        self.planner.update_map(self.get_2d_map())

    def get_pts(self, pts):
        return self.prune_point_cloud(self.convert_pts_to_tracking(pts))

    def python_callback(self, pts):
        """
        This is called when the depth camera receives a new set of points via the python api. It is implemented
        as a callback so it can happen in a separate thread.
        It calls a function to extract and filter the points (colors are ignored) and updates the map with points only -
        when using the python API, it should be a points only map.
        """
        if len(pts) < 10:
            return
        self.cam_odom = self.last_cam_odom
        if self.cam_odom is not None:
            self.update_map(self.get_pts(pts))

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
    m._map3d.grid2d = np.load("resources/environment.npy")
    m.publish_vis_dense(extra_pts=2)


if __name__ == '__main__':
    main()
