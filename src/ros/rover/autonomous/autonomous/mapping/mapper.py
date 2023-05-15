#!/usr/bin/python3

__package__ = "autonomous"

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team. Base Mapper class that
maps the 2d surroundings by simply extracting
layers from the 3d map. Extended by other Mappers
with more evolved obstacle detection algorithms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: points_grid
TOPICS:
  - Subscriber: /camera/depth/color/points [sensor_msgs.msg.PointCloud2]
  - Subscriber: /t265/odom/sample [nav_msgs.msg.Odometry]

SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	autonomous
AUTHOR(S):	Lucas, Kelly, Kelvin, Amesh, Liam, Max
CREATION:	27/09/2021
EDITED:		17/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - a lot 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import numpy as np
import time
import logging

import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration
from tf2_ros import TransformListener, Buffer
from sensor_msgs.msg import PointCloud2

class Mapper(Node):
    def __init__(self, height=5, planner=None, resolution=0.1, name='mapper', camera=False):
        super().__init__(name)
        self.initialised = False

        self.has_color = False



def main(args=None):
    rclpy.init(args=args)
    # reset_cameras.reset_cameras()
    subscriber = Mapper()
    rclpy.spin(subscriber)
    subscriber.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
