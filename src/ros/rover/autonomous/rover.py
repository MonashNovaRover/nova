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

def PointField(name, offset, datatype, count):
    return PF(name=name, offset=offset, datatype=datatype, count=count)


def create_cloud_color(points):
    """
    :param points: list of points with colors, where each item (point) is a 6-tuple of (x, y, z, r, g, b, 0)
    """

    # fields being passed into the actual message
    fields_for_msg = [PointField('x', 0, PF.FLOAT32, 1),
              PointField('y', 4, PF.FLOAT32, 1),
              PointField('z', 8, PF.FLOAT32, 1),
              PointField('rgb', 16, PF.FLOAT32, 1)]

    # fields for cloud_point2 to do the nasty struct conversion
    fields_for_parse = [PointField('x', 0, PF.FLOAT32, 1),
              PointField('y', 4, PF.FLOAT32, 1),
              PointField('z', 8, PF.FLOAT32, 1),
              PointField('r', 16, PF.UINT8, 1),
              PointField('g', 17, PF.UINT8, 1),
              PointField('b', 18, PF.UINT8, 1),
              PointField('x', 19, PF.UINT8, 1),
              ]
    
    header = Header()
    header.frame_id = "camera_depth_optical_frame"
    
    t = Time()
    t.sec = int(time.time())
    t.nanosec = 0  # fix this lol
    header.stamp = t 
    
    cloud = cloud_point2.create_cloud(header, fields_for_parse, points)
    
    cloud.fields = fields_for_msg
    
    return cloud
    
class RoverCloud(Node):
    def __init__(self):
        super().__init__("cloud_pub_test")
        self.publisher = self.create_publisher(PointCloud2, "test_cloud", 10)
        
        mesh = o3d.io.read_triangle_mesh("rover.ply")
        pcd = mesh.sample_points_uniformly(number_of_points=20000)
        pts = np.asarray(pcd.points)
        
        # colors = np.asarray(pcd.colors)
        # pts = np.concatenate((pts, np.zeros((len(pts), 4)).astype(int)), axis=1)
        
        # get maximum x coord
        
        print("max x: " + str(max([pt[0] for pt in pts])))
        
        pts = pts / 1344
        
        pts = pts[:, [0, 2, 1]]
        
        pts = pts - np.array([.4, .3, -0.11])
        
        self.og_pts = pts
        
        self.subscriber_points = self.create_subscription(Odometry, '/T265/odom/sample', self.callback, 10)

    def callback(self, msg):

        mesh = o3d.geometry.TriangleMesh.create_coordinate_frame()

        #o3d.visualization.draw_geometries([mesh])

        x = msg.pose.pose.position.x
        y = msg.pose.pose.position.y
        z = msg.pose.pose.position.z
        xquar = msg.pose.pose.orientation.x
        yquar = msg.pose.pose.orientation.y
        zquar = msg.pose.pose.orientation.z
        wquar = msg.pose.pose.orientation.w
        
        self.pub_rover_at([x, y, z])
        

    def pub_rover_at(self, coords):
        """
        ok, so rviz defines x y forward and x right, as opposed to x forward and y right, so we need to swap x and y just before vis

        """
        
        pts = self.og_pts + coords

        # final transformations JUST for visualization
        pts = pts[:, [1, 0, 2]]

        pts = [pt.tolist() + [0,77,255,0] for pt in pts]
        
        self.pub(pts)

    def pub(self, points):
        pc2 = create_cloud_color(points)
        print(type(pc2))
        self.publisher.publish(pc2)
    

if __name__ == "__main__":
    rclpy.init(args=None)
    cloud = RoverCloud()
    rclpy.spin(cloud)
    rclpy.shutdown()

