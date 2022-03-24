__package__ = "autonomous"
#!/usr/bin/python3
"""
Convert rover as .ply file to pointcloud
"""

import open3d as o3d
import numpy as np
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
import vis.pc_pub as pc_pub
import math_utils.transform as transform
from config.ros_config import tracking_pose_topic
from core.msg import AlvarMarker


class RoverCloud(Node):
    def __init__(self, mode="from_file"):
        super().__init__("ar_cloud_pub")
        # create the point-cloud publisher (this is how we will visualise the rover)
        self.pc_pub = pc_pub.PCPub("ar_cloud")

        # this is the thing we publish. It should be a set of points where if there is a (0,0,0) translation,
        # the rover is just sitting on the ground at the origin
        pts = []
        for x in range(0, 100):
            for y in range(0, 100):
                pts.append([x, y, 0])
        self.tag_pts = np.asarray(pts) / 1000.0 - 0.05
        self.subscriber_points = self.create_subscription(Odometry, "autonomous/ar_tag", self.callback, 10)

    def callback(self, msg):
        self.pub_tag_at(msg)

    def pub_tag_at(self, msg):
        """
        For publishing the rover based on a center wheel bsed (0,0,0)
        """
        x = msg.pose.pose.position.x
        y = msg.pose.pose.position.y
        z = msg.pose.pose.position.z
        
        # applies transformation then translation
        pts = self.tag_pts
        mat = transform.get_pc_rotation_matrix(msg)
        pts = np.matmul(mat, pts.transpose()).transpose()
        pts = pts + [x, y, z]

        # the rover signature orange^tm
        pts = [pt.tolist() + [0, 77, 255, 0] for pt in pts]
        self.pc_pub.pub(pts)
        
    def pub_tag_at_coords(self, coords):
        pts = self.tag_pts
        pts = pts + coords
        # the rover signature orange^tm
        pts = [pt.tolist() + [0, 77, 255, 0] for pt in pts]
        self.pc_pub.pub(pts)


if __name__ == "__main__":
    rclpy.init(args=None)
    cloud = RoverCloud()
    rclpy.spin(cloud)
    rclpy.shutdown()

