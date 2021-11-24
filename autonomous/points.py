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

        self.vis = Visualizer()
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

        o3d.visualization.draw_geometries([pointSet])
        
        self.vis.update_geometry([pointset])
        self.vis.poll_events()
        self.vis.update_renderer()

        time.sleep(.5)

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
