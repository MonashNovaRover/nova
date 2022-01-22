#!/usr/bin/python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
AUTHOR(S):	Lucas, Kelly, Kelvin, Amesh, Liam
CREATION:	27/09/2021
EDITED:		8/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - a lot 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2
import pc_converter as pc2
import transform
from sensor_msgs.msg import PointField
from nav_msgs.msg import Odometry
from map2d_container import Map2DContainer
from grid_3d import Grid3D
import matplotlib.pyplot as plt
import numpy as np
import pc_pub


plot = False


class Mapper(Node):
    def __init__(self, map2d, length=20, width=20, height=6, resolution=0.05):

        # init node with node name points
        super().__init__('points_grid')
        self.subscriber_tracking = self.create_subscription(Odometry, '/T265/odom/sample', self.tracking_callback, 100)
        
        self.subscriber_points = self.create_subscription(PointCloud2, '/D400/depth/color/points', self.points_callback, 10)

        # is_listener attr to be used to be return publisher
        self.map2d = Map2DContainer(is_publisher=True)

        # constants for pruning the point-clouds
        self.max_dist = 3.5

        self.last_msg = None

        # limiting the the field of view to 4 degrees up and down to reduce noisy data points
        # 0.349066 radians == 20 degrees
        self.max_angle = 0.349066
        
        self.map2d = map2d
        if not map2d:
            self.map2d = Map2DContainer(is_ros=False, length=length, width=width)

        self.length = length
        self.width = width
        self.height = height
        self.resolution = resolution
        self.map3d = Grid3D(self.length, self.width, self.height, self.resolution)

        self.msg = None
        
        # for visualising the map
        self.pc_pub = pc_pub.PCPub("map_cloud")

        if plot:
            plt.ion()
            plt.show()

    # use the update from 3d method form Map2D
    def generate_map2d(self):
        return self.map2d.update_from_3d()

    def get_transform(self):
        return transform.get_pc_transformation(self.msg)
    
    def get_translation(self):
        """
        :return: (3) ndarray for positional translation
        """
        x = self.msg.pose.pose.position.x
        y = self.msg.pose.pose.position.y
        z = self.msg.pose.pose.position.z
        return np.array([x, y, z])

    def get_points_and_colors(self, msg):
        """
        Gets points and colors as ndarrays from PointCloud2 data from the D415 depth camera.
        Also transforms into the Nova left handed coordinate system.
        :param msg: PointCloud2
        :return: (n, 3) ndarray, (n, 3) ndarray
        """

        # we need to re-set the field names to extract the unsigned ints from the msg type (one for r, g, b)
        msg.fields = msg.fields[0:3]
        msg.fields.append(PointField(name="r", offset=16, datatype=2, count=1))
        msg.fields.append(PointField(name="g", offset=17, datatype=2, count=1))
        msg.fields.append(PointField(name="b", offset=18, datatype=2, count=1))

        # 1. Parse raw point-cloud data into array of (x, y, z) tuples
        arr = list(pc2.read_points(msg, field_names=("x", "y", "z", "r", "g", "b"), skip_nans=True))

        # 2. Wrap the point-cloud array in a numpy array
        np_arr = np.array(arr)
        
        # 3. Split into points (x, y, z) and colors (r, g, b) 
        pts = np_arr[:, 0:3]
        colors = np_arr[:, 3:6] / 255.0

        # 4. Swap red and blue (for some reason it's not stored how it should be)
        colors = colors[:, [0, 1, 2]]
        
        # 5. Transform to tracking camera coordinates

        # converting from (x=right, y=down, z=forward) -> (x=forward, y=right, z=up)
        pts = pts[:, [2, 0, 1]]
        pts[:, 2] = -pts[:, 2]
        pts[:, 1] = -pts[:, 1]

        # 6. only taking every 10th value (cos 2 much data)
        colors = colors[list(range(0, len(colors), 10))]
        pts = pts[list(range(0, len(pts), 10))]
        
        # 7. further pruning out points which are either beyond the max dist, or are outside the max angle
        indexes = (self.row_norm(pts) < self.max_dist) & (abs(np.arctan(pts[:, 1] / pts[:, 0])) < self.max_angle) \
                  & (abs(np.arctan(pts[:, 2] / pts[:, 0])) < self.max_angle)
        
        pts = pts[indexes]
        colors = colors[indexes]

        return pts, colors
    
    @staticmethod
    def row_norm(pts):
        """
        Efficient numpy way of doing euclidean distance over each row 
        (just takes all the values in the column, which will be 3 for our purposes, and calculates the lenght)
        :param pts: (n, 3) array of points
        :return: what we need to
        """
        return np.sum(np.abs(pts) ** 2, axis=-1) ** (1.0 / 2)
    
    def points_callback(self, msg):
        # update 3D map
        self.msg = self.last_msg
        pts, colors = self.get_points_and_colors(msg)
        
        if pts.shape[0] < 10:
            return

        if self.msg:
            mat = self.get_transform()
            pts = np.matmul(mat, pts.transpose()).transpose()
            pts = pts + self.get_translation()
        
        colors = colors * 255
        self.map3d.add_pc(pts, colors)
        pts, colors = self.map3d.get_as_pc()
        self.pc_pub.pub_pts_colors(pts, colors)

        # update 2D map
        # ----------------------- WARNING: JANK ----------------------
        self.map2d.grid = self.map3d.get_slices(self.msg, 0.1, 0.1)
        
        if plot:
            plt.imshow(np.flip(self.map2d.grid, axis=0))
            plt.draw()
            plt.pause(0.01)

    def publish_vis(self):
        pts, colors = self.map3d.get_as_pc()
        self.pc_pub.pub_pts_colors(pts, colors)
    
    def publish_vis_dense(self, extra_pts=1):
        pts, colors = self.map3d.get_as_pc()
        colors = colors + [254,254,254]
        print(pts)
        pts_dense = pts[:]
        colors_dense = colors[:]
        self.pc_pub.pub_pts_colors(pts, colors)
        min_z = np.min(pts[:,2])
        max_z = np.max(pts[:,2])
        colors[:,2] = 254 * abs(pts[:,2]) / max_z
        colors[:,0] = 200 * (max_z - abs(pts[:,2])) / max_z
        colors[:,1] = 100 * (max_z - abs(pts[:,2])) / max_z
        for x in range(extra_pts + 1):
            x_shift = [(self.resolution / (extra_pts + 1)) * x, 0, 0]
            for y in range(extra_pts + 1):
                y_shift = [0, (self.resolution / (extra_pts + 1)) * y, 0]
                for z in range(extra_pts + 1):
                    z_shift = [0, 0, (self.resolution / (extra_pts + 1)) * z]
                    pts_dense = np.concatenate((pts_dense, pts + x_shift + y_shift + z_shift))
                    colors_dense = np.concatenate((colors_dense, colors))
        self.pc_pub.pub_pts_colors(pts_dense, colors_dense)

    def tracking_callback(self, msg):
        self.last_msg = msg


def position_callback(msg):
    """
    Parses positional data, calculates the average value and publishes
    it to the topic /obstacle_proximity.
    """
    print(msg.pose.pose.position.x)


def main(args=None):
    rclpy.init(args=args)
    subscriber = Mapper(None)
    rclpy.spin(subscriber)
    subscriber.destroy_node()
    rclpy.shutdown()


def vis():
    rclpy.init()
    m = Mapper(None, length=10, width=10, height=5, resolution=.2)
    m.map3d.map = np.load("resources/environment.npy")
    m.publish_vis_dense(extra_pts=2)


if __name__ == '__main__':
    main()
