#!/usr/bin/python3

"""
The class PCPub is meant to be used as an easy to create publisher of PointCloud2 data, so we can visualize a range 
of 3D data in RVIZ in the same global frame.
"""

import numpy as np
from autonomous.utils import cloud_point2
from builtin_interfaces.msg import Time
import time
from rclpy.node import Node
from std_msgs.msg import Header
from sensor_msgs.msg import PointCloud2, PointField as PF


def PointField(name, offset, datatype, count):
    return PF(name=name, offset=offset, datatype=datatype, count=count)

class PCPub(Node):
    """
    This creates a node which has the sole purpose of publishing transformed point-clouds to be viewed in RVIZ. 
    Therefore, the coordinates are a bit flipped to fit with rviz's stupid coordinate frame.
    """
    def __init__(self, node_name):
        super().__init__(node_name)
        self.publisher = self.create_publisher(PointCloud2, node_name + "/cloud", 10)

    def pub(self, points):
        # final transformations JUST for visualization
        points = np.array(points)[:, :]
        points = [pt[0:3].tolist() + pt[3:7].astype(int).tolist() for pt in points]
        pc2 = create_cloud_color(points)
        self.publisher.publish(pc2)
    
    def pub_pts_colors(self, pts, colors):
        self.pub(np.concatenate((pts, colors, np.zeros((len(pts), 1))), axis=1))

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

