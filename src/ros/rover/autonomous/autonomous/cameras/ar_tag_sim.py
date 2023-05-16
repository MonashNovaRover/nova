#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Listen to /tf transforms and store a 
    number of imaginary AR tags in the world.
    Publish all those that would be visible in
    the rover's current field of view.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ar_sim_node
TOPICS:
  - subscriber: 
        /tf [TransformStamped]
  - publisher: 
        /AlvarMarkers [/ar_trager/tags]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    autonomous
AUTHOR(S):	Max Tory
CREATION:	16/05/2023
EDITED:		16/05/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from tf2_ros import TransformListener, Buffer

# msg imports
from core.msg import AlvarMarkers, AlvarMarker
from geometry_msgs.msg import TransformStamped, Pose, Point, Quaternion

# nova import
from autonomous.math_utils import transform
from autonomous.math_utils.controller_math import distance
from autonomous.config.runtime_params import max_point_depth, max_fov_horizontal, min_ar_distance

# python imports 
from typing import Tuple
import numpy as np
import logging

# AR tag poses in map frame
tags = {
    0: Pose(position=Point(x=25.3, y=-10.4, z=0.), orientation=Quaternion(x=0., y=0., z=0., w=1.)),
    1: Pose(position=Point(x=68.2, y=-39., z=0.), orientation=Quaternion(x=0., y=0., z=0., w=1.)),
    2: Pose(position=Point(x=75.1, y=20., z=0.), orientation=Quaternion(x=0., y=0., z=0., w=1.)),
    3: Pose(position=Point(x=102.3, y=56.21, z=0.), orientation=Quaternion(x=0., y=0., z=0., w=1.)),
    4: Pose(position=Point(x=100.1, y=55.9, z=0.), orientation=Quaternion(x=0., y=0., z=0., w=1.)),
    5: Pose(position=Point(x=5., y=0., z=0.), orientation=Quaternion(x=0., y=0., z=0., w=1.)),
}

class TemplateNode(Node):

    def __init__(self):
        super().__init__("TemplateNode")

        self.tf_buffer = Buffer()
        self.tf_listerner = TransformListener(self.tf_buffer, self, spin_thread=True)

        self.pub_markers = self.create_publisher(AlvarMarkers, "/ar_tracker/tags", 10)
        timer_period = 0.1  # run the timer 10 times per second
        self.timer_check_tags = self.create_timer(timer_period, self.cb_ar_tags)

    def in_fov(self, pose: Pose) -> bool:
        """
        Check if a pose is in the rover's field of view
        """
        return abs(np.arctan2(pose.position.y, pose.position.x)) < max_fov_horizontal \
            and min_ar_distance < distance((0, 0), (pose.position.x, pose.position.y)) < max_point_depth \
            and pose.position.x > 0

    def cb_ar_tags(self):
        """
        Called every timer_period. 
        Publishes all AR tags that are visible in the rover's current field of view.
        """
        try:
            # Get the transform from the camera to the map
            tf_map_to_cam : TransformStamped = self.tf_buffer.lookup_transform("d435_1_forward", "map", Time())
        except Exception as e:
            self.get_logger().warn(f"Failed to get transform from map to camera: {e}")
            return

        msg = AlvarMarkers()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = "d435_1_forward"
        for tag in tags:
            cam_frame_pose = transform.transform_pose(tags[tag], tf_map_to_cam.transform)
            if self.in_fov(cam_frame_pose):
                new_marker = AlvarMarker()
                new_marker.pose.pose = cam_frame_pose
                new_marker.pose.header = msg.header
                new_marker.tag_id = tag
                msg.markers.append(new_marker)
        self.pub_markers.publish(msg)

def main():
    rclpy.init()
    template_node = TemplateNode()
    rclpy.spin(template_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
