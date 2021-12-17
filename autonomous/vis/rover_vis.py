#!/usr/bin/python3

"""
Convert rover as .ply file to pointcloud
"""

import open3d as o3d
import numpy as np
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
import PCPub
from autonomous.utils import transform


class RoverCloud(Node):
    def __init__(self, mode="from_file"):
        
        super().__init__("cloud_pub_test")
        
        if mode == "from_pc":

            # create the point-cloud publisher (this is how we will visualise the rover)
            self.pc_pub = PCPub.PCPub("rover_cloud")
            
            # import the rover from mesh file
            mesh = o3d.io.read_triangle_mesh("../resources/rover.ply")
            pcd = mesh.sample_points_uniformly(number_of_points=20000)
            pts = np.asarray(pcd.points)
            
            # the following are a bunch of (rough) transformations which will put the rover in a reasonable position
            pts = pts / 1344
            pts = pts[:, [0, 2, 1]]
            #pts = pts - np.array([.4, .3, -0.11])
            pts = pts + np.array([0, 0, 0])

            # analysis of raw points:
            # given the rover's max length is 1080 m, but the diff between min and max x coords in the cloud is 0.8172089672994012,
            # we can scale up the rover by:
            # pts = pts * (1080 / 0.8172089672994012)

            pts = pts * (1.080 / 0.8172089672994012)
            
            pts = pts + np.array([-.245,-0.39139580577343547,0.15520411544352622])

            # this is the thing we publish. It should be a set of points where if there is a (0,0,0) translation, the rover is 
            # just sitting on the ground at the 
            self.origin_rover_pts = pts
             
            # save to file
            np.save("rover", pts)
            
            self.subscriber_points = self.create_subscription(Odometry, '/T265/odom/sample', self.callback, 10)
       
        else:
            # create the point-cloud publisher (this is how we will visualise the rover)
            self.pc_pub = PCPub.PCPub("rover_cloud")
            
            # this is the thing we publish. It should be a set of points where if there is a (0,0,0) translation, the rover is 
            # just sitting on the ground at the origin
            self.origin_rover_pts = np.load("../resources/rover.npy")
            
            self.subscriber_points = self.create_subscription(Odometry, '/T265/odom/sample', self.callback, 10)

    def callback(self, msg):
        self.pub_rover_cameras_at(msg)

    def pub_rover_at(self, msg):
        
        """
        For publishing the rover based on a center wheel bsed (0,0,0)
        """

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
        
    def pub_rover_cameras_at(self, msg):
        """
        For publishing the rover where the cameras represent (0,0,0)
        """
        
        x = msg.pose.pose.position.x
        y = msg.pose.pose.position.y
        z = msg.pose.pose.position.z
        
        # applies transformation then translation
        pts = self.origin_rover_pts

        # apply translation to make cameras at [0,0,0]
        pts = pts + [-0.4, 0.0, -0.4]        

        mat = transform.get_pc_transformation(msg)
        pts = np.matmul(mat, pts.transpose()).transpose()
        pts = pts + [x, y, z]

        # the rover signature orange^tm
        pts = [pt.tolist() + [0,77,255,0] for pt in pts]
        self.pc_pub.pub(pts)

    def pub_rover_at_coords(self, coords):
        pts = self.origin_rover_pts
        pts = pts + coords 

        # the rover signature orange^tm
        pts = [pt.tolist() + [0,77,255,0] for pt in pts]
        self.pc_pub.pub(pts)

if __name__ == "__main__":
    rclpy.init(args=None)
    cloud = RoverCloud()
    rclpy.spin(cloud)
    rclpy.shutdown()

