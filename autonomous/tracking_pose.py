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
from nav_msgs.msg import Odometry
from core.msg import RoverPose
import math
import sys


class T265(Node):
    def __init__(self):

        # init node with node name points
        super().__init__('T265')
        self.subscriber_tracking = self.create_subscription(Odometry, '/T265/odom/sample', self.tracking_callback, 100)
        
        self.publisher = self.create_publisher(RoverPose, "autonomous/pose", 10)

    def tracking_callback(self, msg):
        pose = RoverPose()
        pose.y = msg.pose.pose.position.x
        pose.x = -msg.pose.pose.position.y
        # calculate yaw - convert from quaternion to euler
        data = msg.pose.pose
        qx = data.orientation.x
        qy = data.orientation.y
        qz = data.orientation.z
        qw = data.orientation.w

        yaw = -math.atan2(2.0*(qx*qy + qw*qz), qw*qw + qx*qx - qy*qy - qz*qz)
        yaw = (yaw if yaw > 0 else 2.0 * math.pi + yaw) + 0
        yaw = yaw if yaw <= math.pi * 2 else yaw - math.pi * 2
        
        pose.yaw = yaw
        sys.stdout.write("\r" + "x: " + str(round(pose.x, 4)).ljust(7) + " | y: " + str(round(pose.y, 4)).ljust(7)
                         + " | yaw: " + str(round(pose.yaw, 4)).ljust(7))
        sys.stdout.flush()
        self.publisher.publish(pose)


def main():
    rclpy.init(args=None)
    t265 = T265()
    rclpy.spin(t265)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
