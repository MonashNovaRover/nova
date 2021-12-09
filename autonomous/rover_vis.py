#!/usr/bin/python3

"""
convert rover as .ply file to pointcloud
"""

import open3d as o3d
import numpy as np
import cloud_point2
from builtin_interfaces.msg import Time
import time
import rclpy
from rclpy.node import Node
from std_msgs.msg import Header
from sensor_msgs.msg import PointCloud2, PointField as PF
from nav_msgs.msg import Odometry
import PCPub
import transform

class RoverCloud(Node):
    def __init__(self):
        
        super().__init__("cloud_pub_test")
        
        # create the point-cloud publisher (this is how we will visualise the rover)
        self.pc_pub = PCPub.PCPub("rover_cloud")
        
        # import the rover from mesh file
        mesh = o3d.io.read_triangle_mesh("rover.ply")
        pcd = mesh.sample_points_uniformly(number_of_points=20000)
        pts = np.asarray(pcd.points)
        
        # the following are a bunch of transformations which will put the rover in a reasonable position
        pts = pts / 1344
        pts = pts[:, [0, 2, 1]]
        #pts = pts - np.array([.4, .3, -0.11])
        pts = pts - np.array([.4, .3, .6])
        self.origin_rover_pts = pts
        
        self.subscriber_points = self.create_subscription(Odometry, '/T265/odom/sample', self.callback, 10)

    def callback(self, msg):
        self.pub_rover_at(msg)
        

    def pub_rover_at(self, msg):
        
        x = msg.pose.pose.position.x
        y = msg.pose.pose.position.y
        z = msg.pose.pose.position.z
        
        # applies transformation then translation
        pts = self.origin_rover_pts
        mat = transform.get_pc_transformation(msg)
        pts = np.matmul(mat, pts.transpose()).transpose()
        pts = pts + [x, y, z]

        # the rover signature orange^tm
        pts = [pt.tolist() + [0,77,255,0] for pt in pts]
        self.pc_pub.pub(pts)

if __name__ == "__main__":
    rclpy.init(args=None)
    cloud = RoverCloud()
    rclpy.spin(cloud)
    rclpy.shutdown()

