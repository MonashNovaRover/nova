#!/usr/bin/python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
A proximity sensor that takes in positional data and calculates 
the average "forward" value of points within certain 3D bounds.
Specifically, a forward range of 0.3 - 3.0 and a vertical
and horizontal range of -0.15 - 0.15. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: obstacle_detector
TOPICS:
  - /D435/depth/color/points [sensor_msgs.msg.PointCloud2]
  - /obstacle_proximity [Float32]
SERVICES:
  - 
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	task
AUTHOR(S):	kelly
CREATION:	27/09/2021
EDITED:		30/09/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
from std_msgs.msg import String
from sensor_msgs.msg import PointCloud2
import cloud_point2 as pc2
from std_msgs.msg import Float32
from nav_msgs.msg import Odometry
import open3d as o3d
import struct
import time
import numpy as np
from open3d.visualization import Visualizer

#---- Subscribes to the topic /D435... and publishes the average Z value
class SubscriberNode(Node):
    def __init__(self):
        super().__init__('obstacle_detector')
        self.subscriber_points = self.create_subscription(PointCloud2, '/D435/depth/color/points',self.points_callback, 10)
        
        # self.subscriber_tracking = self.create_subscription(Odometry, "/T265/odom/sample", self.position_callback, 10)

        #o3d.utility.set_verbosity_level(o3d.utility.VerbosityLevel.Debug)
        self.vis = o3d.visualization.Visualizer() 
        self.vis.create_window()


    def points_callback(self, msg):
        """
        Parses positional data, calculates the average value and publishes
        it to the topic /obstacle_proximity.
        """
        data = msg.data
        arr = []
        for p in pc2.read_points(msg, field_names=("x", "y", "z"), skip_nans=True):  
            arr.append(p)

        np_arr = np.array(arr)

        pointSet = o3d.geometry.PointCloud()

        pointSet.points = o3d.utility.Vector3dVector(np_arr)
        
        # downsampling test to pointcloud
        #print("Downsample the point cloud with a voxel of 0.05")
        #downpcd = pointSet.voxel_down_sample(voxel_size=0.1) #voxel 
        #o3d.visualization.draw_geometries([downpcd], zoom=0.3412, front=[0.4257, -0.2125, -0.8795], lookat=[2.6172, 2.0475, 1.532], up=[-0.0694, -0.9768, 0.2024])

        # voxelisation test
        N = 2000
        # fit to unit cube
        pointSet.scale(1 / np.max(pointSet.get_max_bound() - pointSet.get_min_bound()),
            center=pointSet.get_center())
        pointSet.colors = o3d.utility.Vector3dVector(np.random.uniform(0, 1, size = (N, 3))) # colour is determined by the average of all the points within the voxel
        #o3d.visualization.draw_geometries([pointSet])

        voxel_grid = o3d.geometry.VoxelGrid.create_from_point_cloud(pointSet, voxel_size = 0.05)
        #o3d.visualization.draw_geometries([voxel_grid])


        # seems to loop forever using this method so it never continues to the next point cloud. Thus non-blocking visulaization!
        #o3d.visualization.draw_geometries([pointSet],   
                               #   zoom=0.3412,
                               #   front=[0.4257, -0.2125, -0.8795],
                               #   lookat=[2.6172, 2.0475, 1.532],
                               #   up=[-0.0694, -0.9768, 0.2024])

        # Non-blocking visualisation of point cloud, very laggy
        self.vis.add_geometry(voxel_grid)
        self.vis.update_geometry(voxel_grid)
        self.vis.poll_events()
        self.vis.update_renderer()
        
        #time.sleep(.5)


    # example code given for non-blocking visualization
    def testfunc(self, pc1, pc2):
        o3d.utility.set_verbosity_level(o3d.utility.VerbosityLevel.Debug)
        source = self.pc1.voxel_down_sample(voxel_size=0.02)
        target = self.pc2.voxel_down_sample(voxel_size=0.02)
        trans = [[0.862, 0.011, -0.507, 0.0], [-0.139, 0.967, -0.215, 0.7],
                [0.487, 0.255, 0.835, -1.4], [0.0, 0.0, 0.0, 1.0]]
        source.transform(trans)

        flip_transform = [[1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1]]
        source.transform(flip_transform)
        target.transform(flip_transform)

        vis = o3d.visualization.Visualizer()
        vis.create_window()
        vis.add_geometry(source)
        vis.add_geometry(target)
        threshold = 0.05
        icp_iteration = 100
        save_image = False

        for i in range(icp_iteration):
            reg_p2l = o3d.pipelines.registration.registration_icp(
                source, target, threshold, np.identity(4),
                o3d.pipelines.registration.TransformationEstimationPointToPlane(),
                o3d.pipelines.registration.ICPConvergenceCriteria(max_iteration=1))
            source.transform(reg_p2l.transformation)
            vis.update_geometry(source)
            vis.poll_events()
            vis.update_renderer()
            if save_image:
                vis.capture_screen_image("temp_%04d.jpg" % i)
        vis.destroy_window()


    def position_callback(self, msg):
        """
        Parses positional data, calculates the average value and publishes
        it to the topic /obstacle_proximity.
        """
        print(msg.pose.pose.position.x)

#---- Set up parameters and nodes and start the ROS class
def main(args = None):
    rclpy.init(args = args)
    subscriber = SubscriberNode()
    rclpy.spin(subscriber)
    subscriber.destroy_node()
    rclpy.shutdown

if __name__ == '__main__':
    main()