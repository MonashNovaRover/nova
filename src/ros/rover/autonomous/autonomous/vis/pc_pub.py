__package__ = "autonomous"
#!/usr/bin/python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The class PCPub is meant to make it east to publish PointCloud2 data, so we can visualize a range
of 3D data in RVIZ in the same global frame, as well as publish raw point-clouds from the depth camera
for later rosbag use.

Coordinate system:
------------------
This class publishes point-clouds exactly as it receives them in self.pub(points), where points is a
numpy array with shape (n, 6) corresponding to x, y, z, r, g, b

NODE: points_grid

TOPICS:
  - /D435/depth/color/points [sensor_msgs.msg.PointCloud2]
  OR (can change based on BAG):
  - /D400/depth/color/points [sensor_msgs.msg.PointCloud2]
  - /T265/odom/sample

SERVICES:
  - 
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	autonomous
AUTHOR(S):	Liam
CREATION:	27/09/2021
EDITED:		8/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO: work on header data:
    - adding millisecond data
    - what is the best frame to set this to?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import numpy as np
from config.runtime_params import pub_scale

import vis.pc_converter as pc_converter
from rclpy.node import Node
from std_msgs.msg import Header
from sensor_msgs.msg import PointCloud2, PointField as PF


def point_field(name, offset, datatype, count):
    return PF(name=name, offset=offset, datatype=datatype, count=count)


class PCPub(Node):
    def __init__(self, node_name, frame_id="d435_1", scale=pub_scale):
        super().__init__(node_name)
        self.publisher = self.create_publisher(PointCloud2, node_name + "/cloud", 10)
        self.scale = scale
        self.frame_id = frame_id

    def pub(self, points):
        # final transformations JUST for visualization
        points = np.array(points)
        points[:, 0:3] = points[:, 0:3] * self.scale
        points = [pt[0:3].tolist() + pt[3:7].astype(int).tolist() for pt in points]

        pc2 = self.create_cloud_color(points)
        pc2.header.stamp = self.get_clock().now().to_msg()
        pc2.header.frame_id = self.frame_id
        self.publisher.publish(pc2)

    def pub_pts_colors(self, pts, colors):
        """
        :param pts: np.array((n, 3)) -- referring to x, y, z
        :param colors: np.array((n, 3)) -- referring to r, g, b
        """
        self.pub(np.concatenate((pts, colors, np.zeros((len(pts), 1))), axis=1))


    def create_cloud_color(self, points):
        """
        :param points: list of points with colors, where each item (point) is a 6-tuple of (x, y, z, r, g, b, 0)
        """
        # fields being passed into the actual message
        fields_for_msg = [point_field('x', 0, PF.FLOAT32, 1),
                        point_field('y', 4, PF.FLOAT32, 1),
                        point_field('z', 8, PF.FLOAT32, 1),
                        point_field('rgb', 16, PF.FLOAT32, 1)]

        # fields for cloud_point2 to do the nasty struct conversion
        fields_for_parse = [point_field('x', 0, PF.FLOAT32, 1),
                            point_field('y', 4, PF.FLOAT32, 1),
                            point_field('z', 8, PF.FLOAT32, 1),
                            point_field('r', 16, PF.UINT8, 1),
                            point_field('g', 17, PF.UINT8, 1),
                            point_field('b', 18, PF.UINT8, 1),
                            point_field('x', 19, PF.UINT8, 1),
                            ]
        
        header = Header()
        header.frame_id = self.frame_id
        header.stamp = self.get_clock().now().to_msg()
        
        cloud = pc_converter.create_cloud(header, fields_for_parse, points)
        cloud.fields = fields_for_msg
        return cloud
