#!/usr/bin/python3

__package__ = "autonomous"

"""
Convert rover as .ply file to pointcloud
"""

import numpy as np
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
import vis.pc_pub as pc_pub


class RoverCloud(Node):
    def __init__(self, mode="from_file"):
        
        super().__init__("cloud_pub_test")
        
        if mode == "from_pc":

            # create the point-cloud publisher (this is how we will visualise the rover)
            self.pc_pub = pc_pub.PCPub("rover_cloud", frame_id="base_link")
            
            # import the rover from mesh file
            mesh = o3d.io.read_triangle_mesh("resources/rover.ply")
            pcd = mesh.sample_points_uniformly(number_of_points=20000)
            pts = np.asarray(pcd.points)
            
            # the following are a bunch of (rough) transformations which will put the rover in a reasonable position
            pts = pts / 1344
            pts = pts[:, [0, 2, 1]]
            # pts = pts - np.array([.4, .3, -0.11])
            pts = pts + np.array([0, 0, 0])

            # analysis of raw points:
            # given the rover's max length is 1080 m, but the diff between min,
            # and max x co-ords in the cloud is 0.8172089672994012,
            # we can scale up the rover by:
            # pts = pts * (1080 / 0.8172089672994012)

            pts = pts * (1.080 / 0.8172089672994012)
            
            pts = pts + np.array([-.245, -0.39139580577343547, 0.15520411544352622])

            # this is the thing we publish. It should be a set of points where if there is a (0,0,0) translation,
            # the rover is  just sitting on the ground at the
            self.origin_rover_pts = pts 
             
            # save to file
            np.save("rover", pts)
        
        # this is what we usually run
        else:
            self.pc_pub = pc_pub.PCPub("rover_cloud")
            self.origin_rover_pts = np.load("resources/rover.npy")

        pts = self.origin_rover_pts
        # Get rover orange
        pts = [pt.tolist() + [0, 77, 255, 0] for pt in pts]
        self.pc_pub.pub(pts)

def main():
    rclpy.init(args=None)
    cloud = RoverCloud()
    rclpy.spin(cloud)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
