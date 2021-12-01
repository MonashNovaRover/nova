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

# off, dynamic, static
VIS = "static"


class SubscriberNode(Node):
    def __init__(self):

        # init node with node name points
        super().__init__('points')
        self.subscriber_points = self.create_subscription(PointCloud2, '/D400/depth/color/points', self.points_callback, 100)
        self.subscriber_tracking = self.create_subscription(Odometry, '/T265/odom/sample', self.tracking_callback, 100)

        self.msg = None
        
        # self.subscriber_tracking = self.create_subscription(Odometry, "/T265/odom/sample", self.position_callback, 10)
        # o3d.utility.set_verbosity_level(o3d.utility.VerbosityLevel.Debug)

        self.grid = None

        if VIS == "dynamic":
            self.vis = o3d.visualization.Visualizer() 
            self.vis.create_window()
            self.vis.get_render_option().load_from_json("view.json")

    def visualize_pc(self, pc):
        self.vis.add_geometry(pc)
        self.vis.update_geometry(pc)
        self.vis.poll_events()
        self.vis.update_renderer()
        self.vis.get_render_option().load_from_json("view.json")

    def visualize(self, voxel_grid):
        # Non-blocking visualisation of point cloud, very lagy
        self.vis.add_geometry(voxel_grid)
        self.vis.update_geometry(voxel_grid)
        self.vis.poll_events()
        self.vis.update_renderer()
        self.vis.get_render_option().load_from_json("view.json")

    def points_callback(self, msg):
        """
        Parses positional data, calculates the average value and publishes
        it to the topic /obstacle_proximity.
        """
        # we need to re-set the field names to extract the unsigned ints from the msg type (one for r, g, b)
        msg.fields = msg.fields[0:3]
        msg.fields.append(PointField(name="r", offset=16, datatype=2, count=1))
        msg.fields.append(PointField(name="g", offset=17, datatype=2, count=1))
        msg.fields.append(PointField(name="b", offset=18, datatype=2, count=1))

        # 1. Parse raw point-cloud data into array of (x, y, z) tuples
        # arr = list(pc2.read_points(msg, field_names=("x", "y", "z"), skip_nans=True))
        arr = list(pc2.read_points(msg, field_names=("x", "y", "z", "r", "g", "b"), skip_nans=True))

        # 2. Wrap the point-cloud array in an o3d.geometry.PointCloud
        np_arr = np.array(arr)
        print(arr[:2])
        pts = np_arr[:, 0:3]

        # points = [p for p in pts[:,1] if abs(p) < 1.5]

        zmin = min(pts[:,2])

        print(zmin)

        colors = np_arr[:, 3:6] / 255.0

        # swap red and blue
        colors = colors[:, [2, 1, 0]]

        colors = colors[(abs(pts[:, 0]) < 2.0) & (abs(pts[:, 1]) < 2.0) & (abs(pts[:, 2]) < 2.0)]
        pts = pts[(abs(pts[:, 0]) < 2.0) & (abs(pts[:, 1]) < 2.0) & (abs(pts[:, 2]) < 2.0)]

        colors = o3d.utility.Vector3dVector(colors)
        pts = o3d.utility.Vector3dVector(pts)

        point_set = o3d.geometry.PointCloud()
        point_set.points = pts
        point_set.colors = colors

        #translated_ps = point_set
        #translated_ps.transform(mat)
        if self.msg:
            x = self.msg.pose.pose.position.x
            y = self.msg.pose.pose.position.y
            z = self.msg.pose.pose.position.z
            xquar = self.msg.pose.pose.orientation.x
            yquar = self.msg.pose.pose.orientation.y
            zquar = self.msg.pose.pose.orientation.z
            wquar = self.msg.pose.pose.orientation.w

            quar = np.array([xquar, yquar, zquar, wquar])

            mat = np.zeros((4, 4))
            mat[:3, :3] = o3d.geometry.get_rotation_matrix_from_quaternion(quar)
            mat[3, 3] = 1

            point_set.transform(mat)
        # fit into unit cube
        point_set.scale(1 / np.max(point_set.get_max_bound() - point_set.get_min_bound()), center=point_set.get_center())

        # colour is determined by the average of all the points within the voxel
        # point_set.colors = o3d.utility.Vector3dVector(np.random.uniform(0, 1, size=(N, 3)))
        if not self.grid:
            self.grid = o3d.geometry.VoxelGrid.create_from_point_cloud(point_set, voxel_size=0.02)
        else:
            voxel = o3d.geometry.VoxelGrid.create_from_point_cloud(point_set, voxel_size=0.02)
            voxel.origin = self.grid.origin
            self.grid += voxel
            print(self.grid)

        print(self.grid)
        # time.sleep(.1)

        if VIS == "dynamic":
            self.visualize(self.grid)
            # self.visualize_pc(point_set)
        elif VIS == "static":
            # axis_pcd = open3d.geometry.TriangleMesh.create_coordinate_frame(size= 0.5)
            # axis_test = open3d.geometry.TriangleMesh.create_coordinate_frame(size=0.1,origin=[0,0,-zmin])
            # open3d.visualization.draw_geometries([point_set]+[axis_test])
            open3d.visualization.draw_geometries([self.grid])
            # open3d.visualization.draw_geometries([voxel_grid])

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
