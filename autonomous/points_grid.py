#!/usr/bin/python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: 
TOPICS:
  
  - /D435/depth/color/points [sensor_msgs.msg.PointCloud2]
  OR (can change based on BAG):
  - /D400/depth/color/points [sensor_msgs.msg.PointCloud2]
  
  - /T265/odom/sample

SERVICES:
  - 
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	task
AUTHOR(S):	Lucas, Kelly, Kelvin, Amesh, Liam
CREATION:	27/09/2021
EDITED:		30/09/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import open3d.cpu.pybind.visualization
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2
import cloud_point2 as pc2
import open3d as o3d
import time
import numpy as np
from sensor_msgs.msg import PointField
from open3d import *
import matplotlib.pyplot as plt
from nav_msgs.msg import Odometry
import ArrayGrid

# off, dynamic, static
VIS = "dynamic"


class SubscriberNode(Node):
    def __init__(self):

        # init node with node name points
        super().__init__('points')
        self.subscriber_points = self.create_subscription(PointCloud2, '/D435/depth/color/points', self.points_callback, 100)
        self.subscriber_tracking = self.create_subscription(Odometry, '/T265/odom/sample', self.tracking_callback, 100)

        self.msg = None

        self.count = 0
        self.start = time.time()

        # self.subscriber_tracking = self.create_subscription(Odometry, "/T265/odom/sample", self.position_callback, 10)
        # o3d.utility.set_verbosity_level(o3d.utility.VerbosityLevel.Debug)

        self.grid = ArrayGrid.ArrayGrid(8, 8, 8, .2)

        if VIS == "dynamic":
            self.vis = o3d.visualization.Visualizer()
            self.vis.create_window()
            self.vis.get_render_option().load_from_json("view.json")

    def visualize_pc(self, pc):
        self.vis.add_geometry(pc)
        self.vis.update_geometry(pc)
        self.vis.poll_events()
        self.vis.update_renderer()

    def get_transform(self):
        xquar = self.msg.pose.pose.orientation.x
        yquar = self.msg.pose.pose.orientation.y
        zquar = self.msg.pose.pose.orientation.z
        wquar = self.msg.pose.pose.orientation.w

        quar = np.array([wquar, xquar, yquar, zquar])

        mat = np.zeros((4, 4))
        mat[:3, :3] = o3d.geometry.get_rotation_matrix_from_quaternion(quar)
        mat[3, 3] = 1
        return mat

    def get_translation(self):
        x = self.msg.pose.pose.position.x
        y = self.msg.pose.pose.position.y
        z = self.msg.pose.pose.position.z
        return np.array([x, y, z])

    def get_points_and_colors(self, msg):
        """
        Gets points and colors as
        :param msg: PointCloud2
        :return:
        """

        # we need to re-set the field names to extract the unsigned ints from the msg type (one for r, g, b)
        msg.fields = msg.fields[0:3]
        msg.fields.append(PointField(name="r", offset=16, datatype=2, count=1))
        msg.fields.append(PointField(name="g", offset=17, datatype=2, count=1))
        msg.fields.append(PointField(name="b", offset=18, datatype=2, count=1))

        # 1. Parse raw point-cloud data into array of (x, y, z) tuples
        # arr = list(pc2.read_points(msg, field_names=("x", "y", "z"), skip_nans=True))
        arr = list(pc2.read_points(msg, field_names=("x", "y", "z", "r", "g", "b"), skip_nans=True))

        # 2. Wrap the point-cloud array in a numpy array
        np_arr = np.array(arr)
        print(arr[:2])
        pts = np_arr[:, 0:3]

        colors = np_arr[:, 3:6] / 255.0

        # swap red and blue
        colors = colors[:, [2, 1, 0]]

        max_dist = 3.0

        colors = colors[(abs(pts[:, 0]) < max_dist) & (abs(pts[:, 1]) < max_dist) & (abs(pts[:, 2]) < max_dist)]
        pts = pts[(abs(pts[:, 0]) < max_dist) & (abs(pts[:, 1]) < max_dist) & (abs(pts[:, 2]) < max_dist)]
        return pts, colors

    def points_callback(self, msg):
        """
        Parses positional data, calculates the average value and publishes
        it to the topic /obstacle_proximity.
        """

        pts, colors = self.get_points_and_colors(msg)

        if pts.shape[0] < 10:
            return

        if self.msg:
            mat = self.get_transform()
            trans = self.get_translation()

            # transform then translate
            pts = np.matmul(mat[:3, :3], pts.transpose()).transpose()
            pts = pts + trans

        self.grid.add_pc(pts, colors)
        pts, colors = self.grid.get_as_pc()

        point_set = o3d.geometry.PointCloud()
        point_set.points = o3d.utility.Vector3dVector(pts)
        point_set.colors = o3d.utility.Vector3dVector(colors)

        if time.time() - self.start > 3.0 and self.count >= 2:
            if VIS == "dynamic":
                self.visualize_pc(point_set)
                # self.visualize_pc(point_set)
            elif VIS == "static":
                open3d.visualization.draw_geometries([point_set])
        self.count += 1
        print(self.count)

    def tracking_callback(self, msg):
        self.msg = msg


def position_callback(msg):
    """
    Parses positional data, calculates the average value and publishes
    it to the topic /obstacle_proximity.
    """
    print(msg.pose.pose.position.x)


def main(args=None):
    rclpy.init(args=args)
    subscriber = SubscriberNode()
    rclpy.spin(subscriber)
    subscriber.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
