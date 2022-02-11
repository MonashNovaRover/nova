__package__ = "autonomous"
#!/usr/bin/python3
  

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: points_grid
TOPICS:
  
  - /camera/depth/color/points [sensor_msgs.msg.PointCloud2]
  - /t265/odom/sample

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
import vis.pc_converter as pc2
import math_utils.transform as transform
from sensor_msgs.msg import PointField
from nav_msgs.msg import Odometry
from mapping.grid_3d import Grid3D
from planning.path_planner import PathPlanner
import matplotlib.pyplot as plt
import numpy as np
import vis.pc_pub as pc_pub
import time
from cameras.depth_camera import DepthCamera
from config.ros_config import tracking_pose_topic

# python | ros
depth_mode = "python"
depth_topic = '/D400/depth/color/points'


class Mapper(Node):
    def __init__(self, length=20, width=20, height=5, resolution=0.1, planner=None, _vis=True):

        # init node with node name points
        super().__init__('points_grid')
        self.subscriber_tracking = self.create_subscription(Odometry, tracking_pose_topic, self.tracking_callback, 100)
        self.planner = planner
        self.vis = _vis

        # constants for pruning the point-clouds
        self.max_dist = 3.5

        self.last_msg = None

        # limiting the the field of view to 4 degrees up and down to reduce noisy data points
        # 0.349066 radians == 20 degrees
        self.max_angle = 0.349066

        self.length = length
        self.width = width
        self.height = height
        self.resolution = resolution

        self.previous_plan = time.perf_counter()

        self.msg = None
        
        # for visualising the map
        self.pc_pub = pc_pub.PCPub("map_cloud")

        if depth_mode == "ros":
            self.subscriber_points = self.create_subscription(PointCloud2, depth_topic, self.ros_points_callback, 10)
            self.map3d = Grid3D(self.length, self.width, self.height, self.resolution, has_color=True)

        elif depth_mode == "python":
            self.camera = DepthCamera(self.python_callback)
            # starts a separate thread which will get depth frames and update mapper
            self.camera.start()
            self.map3d = Grid3D(self.length, self.width, self.height, self.resolution, has_color=False)

    def get_transform(self):
        """
        Indirection (like sleight of hand, but less interesting)
        """
        return transform.get_pc_rotation_matrix(self.msg)
    
    def get_translation(self):
        """
        :return: (3) ndarray for positional translation
        """
        x = self.msg.pose.pose.position.x
        y = self.msg.pose.pose.position.y
        z = self.msg.pose.pose.position.z
        return np.array([x, y, z])

    def extract_layer(self, height_m):
        return self.map3d.extract_z(height_m)

    def get_points_and_colors(self, msg):
        """
        Gets points and colors as ndarrays from PointCloud2 data from the D415 depth camera.
        Also transforms into the Nova left handed coordinate system.
        :param msg: PointCloud2
        :return: (n, 6) ndarray
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

        # put it all in the one array of shape (n, 6)
        points = np.concatenate((pts, colors), axis=1)

        return points
    
    @staticmethod
    def row_norm(pts):
        """
        Efficient numpy way of doing euclidean distance over each row 
        (just takes all the values in the column, which will be 3 for our purposes, and calculates the lenght)
        :param pts: (n, 3) array of points
        :return: what we need to
        """
        return np.sum(np.abs(pts) ** 2, axis=-1) ** (1.0 / 2)

    def update_map_pts_only(self, pts):
        """
        :param pts: np.array(n, 6) - refers to x,y,z,r,g,b
        """

        if pts.shape[0] < 10:
            return

        # transform the points
        if self.msg:
            # print("transforming pc")
            mat = self.get_transform()
            pts = np.matmul(mat, pts.transpose()).transpose()
            pts = pts + self.get_translation()

        # edit both of these to handle non coloured point-clouds
        self.map3d.add_pc_points_only(pts)
        pts = self.map3d.get_as_pc()
        
        if time.perf_counter() - self.previous_plan > 2:
            if self.planner:
                self.previous_plan = time.perf_counter()
                layer = self.extract_layer(2.3)
                print(sum(layer))
                self.planner.get_path(layer.squeeze())

        # setting colors proportional to the height of points - hopefully looks cool!
        if self.vis:
            max_z = 10
            colors = np.array([(abs(pts[:, 2]) + 1 / max_z) * 250.0 % 250, np.full(len(pts), 0), abs(max_z - abs(pts[:,2]) - 1) * 250 % 250]).transpose()
            
            # white mode
            # colors = np.array(np.full((len(pts), 3), 255))
            self.pc_pub.pub_pts_colors(pts, colors.astype(int))

    def update_map(self, pts):
        """
        :param pts: np.array(n, 6) - refers to x,y,z,r,g,b
        """

        if pts.shape[0] < 10:
            return

        colors = pts[:, 3:]

        if self.msg:
            mat = self.get_transform()
            pts = np.matmul(mat, pts.transpose()).transpose()
            pts = pts + self.get_translation()

        colors = colors * 255
        self.map3d.add_pc(pts, colors)
        pts, colors = self.map3d.get_as_pc()
        self.pc_pub.pub_pts_colors(pts, colors)

    def get_pts(self, pts):
        # 1. Transform to tracking camera coordinates

        # converting from (x=right, y=down, z=forward) -> (x=forward, y=right, z=up)
        pts = pts[:, [2, 0, 1]]
        pts[:, 2] = -pts[:, 2]
        pts[:, 1] = -pts[:, 1]

        # 6. only taking every 10th value (cos 2 much data)
        pts = pts[list(range(0, len(pts), 10))]

        # 7. further pruning out points which are either beyond the max dist, or are outside the max angle
        indexes = (self.row_norm(pts) < self.max_dist) & (abs(np.arctan(pts[:, 1] / pts[:, 0])) < self.max_angle) \
                  & (abs(np.arctan(pts[:, 2] / pts[:, 0])) < self.max_angle)

        pts = pts[indexes]
        return pts

    def python_callback(self, msg):
        """
        This is called when the depth camera receives a new set of points via the python api. It is implemented
        as a callback so it can happen in a separate thread.
        It calls a function to extract and filter the points (colors are ignored) and updates the map with points only -
        when using the python API, it should be a points only map.
        """
        self.msg = self.last_msg
        
        # t = time.time()
        self.update_map_pts_only(self.get_pts(msg))


    def ros_points_callback(self, msg):
        self.msg = self.last_msg
        pts, colors = self.get_points_and_colors(msg)
        self.update_map(pts)
        # every 2 seconds we run planning
        if time.perf_counter() - self.previous_plan > 2:
            if self.planner:
                self.previous_plan = time.perf_counter()
                self.planner.get_path(self.extract_layer(2.8))

    def publish_vis_dense(self, extra_pts=1):
        """
        This was more of an experiment, but it's probably completely pointless (hehe)
        """
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
    # print(msg.pose.pose.position.x)
    pass

def main(args=None):
    rclpy.init(args=args)
    # reset_cameras.reset_cameras()
    subscriber = Mapper()
    rclpy.spin(subscriber)
    subscriber.destroy_node()
    rclpy.shutdown()


def vis():
    rclpy.init()
    m = Mapper(length=10, width=10, height=5, resolution=.2)
    m.map3d.map = np.load("resources/environment.npy")
    m.publish_vis_dense(extra_pts=2)


if __name__ == '__main__':
    main()
