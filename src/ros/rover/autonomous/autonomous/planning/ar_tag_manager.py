#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
For managing the state of seen AR tags,
specifically for the URC competition.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ar_tag_manager
TOPICS:
        - Subscribes: /autonomous/ar_tags goals [Waypoints]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        autonomous
AUTHOR(S):      Max Tory, Liam Whittle
CREATION:       07/12/2021
EDITED:         15/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
# ros imports
from rclpy.node import Node
from rclpy.duration import Duration
from geometry_msgs.msg import TransformStamped, Pose
from visualization_msgs.msg import Marker, MarkerArray

# our imports
from core.msg import AlvarMarkers, AutonomousGoal, AlvarMarker, AutonomousGoalArray, AutonomousGoal
#from autonomous.config.runtime_params import *
from autonomous.config.ros_config import auto_goal_topic
from autonomous.math_utils import transform

# regular imports
import numpy as np
import logging
from collections import deque


class ArTagManager(Node):
    """
    Note: this is only a node so it can use a ROS logger
    Manages the state of AR tags. Keeps track of the last 10 AR tag global pose
    """

    def __init__(self, max_tag_id=5, queue_size=10):
        super().__init__('autonomous_ar_tag_manager')
        self.get_logger().set_level(logging.INFO)
        self.max_tag_id = max_tag_id
        self.queue_size = queue_size

        self.marker_color = (0.9, 0.9, 0.9)

        self.goal : AutonomousGoal = None
        self.ar_tag_poses = {
            _id: deque([], maxlen=self.queue_size) for _id in range(self.max_tag_id + 1)
        }
        self.publisher = self.create_publisher(MarkerArray, "/visualisation/markers", 10)

        timer_period = 0.1  # run the timer 10 times per second
        self.create_timer(timer_period, self.pub_last_tags)

        self.get_logger().debug("AR tag manager initialised!")

    def get_marker(self, point, c: tuple, index: int, namespace: str) -> None:
        """
        :params: c is color tuple (r,g,b) between 0 and 1
        """
        msg = Marker()
        msg.pose.position.x = point[0]
        msg.pose.position.y = point[1]
        msg.pose.position.z = 0.5
        msg.pose.orientation.w = 1.0
        msg.type = Marker.CYLINDER
        msg.scale.x = .15
        msg.scale.y = .15
        msg.scale.z = 1.0
        msg.color.r = c[0]
        msg.color.g = c[1]
        msg.color.b = c[2]
        msg.color.a = 1.
        # Namespace - raw messages can be separated from confirmed cubes
        msg.ns = namespace
        msg.id = index
        # Survive for half a second
        return msg

    def pub_last_tags(self):
        """
        Publishes the last goals
        """
        msg = MarkerArray()
        tag : AlvarMarker
        for _id in self.ar_tag_poses:
            if len(self.ar_tag_poses[_id]) == 0:
                continue
            color = self.marker_color
            tag_pos = self.get_average_tag_pose(_id)
            marker : Marker = self.get_marker(tag_pos, color, _id, "tag")
            marker.header.stamp = self.get_clock().now().to_msg()
            marker.header.frame_id = "map"
            msg.markers.append(marker)
        self.publisher.publish(msg)

    def set_goal(self, goal: AutonomousGoal):
        """
        Provides a new goal to the AR tag manager
        """
        self.get_logger().debug(f"received goal: {goal}")
        for _id in goal.tag_ids:
            assert 0 <= _id <= self.max_tag_id, "AR tag id out of range"

        self.goal = goal

    def has_ar_tags(self) -> bool:
        """
        Returns true if the current goal has AR tags, else false
        """
        return not self.goal.type == AutonomousGoal.GOAL_TYPE_COORDINATE

    def has_goal(self) -> bool:
        return self.goal is not None and (self.found_current_gate() or self.found_current_tag())

    def get_average_tag_pose(self, tag_id):
        """
        Gets the average pose of the current AR tag poses
        """
        return np.mean(self.ar_tag_poses[tag_id], axis=0)

    def get_average_goal_pose(self):
        """
        If we have one tag, what is the average vector of that tag?
        If we have two tags, what is their midpoint
        """
        average_tags = [self.get_average_tag_pose(_id) for _id in self.goal.tag_ids]
        return np.mean(average_tags, axis=0)

    def get_gate_normal(self):
        """
        Assuming we have 2 tags, what is the unit vector normal to the line between them
        """
        assert self.goal.type == AutonomousGoal.GOAL_TYPE_GATE, "Goal type is not gate"
        gate_l, gate_r = self.get_average_tag_pose(self.goal.tag_ids[0]),\
                            self.get_average_tag_pose(self.goal.tag_ids[1])

        normal = np.array((gate_l[1] - gate_r[1], gate_r[0] - gate_l[0]))
        unit_normal = normal / np.linalg.norm(normal)
        return unit_normal

    def found_current_tag(self):
        """
        Were we looking for a single AR tag, and if so, have we found it?
        """
        if self.goal.type != AutonomousGoal.GOAL_TYPE_TAG:
            return False
        retval = len(self.ar_tag_poses[self.goal.tag_ids[0]]) > 0
        self.get_logger().debug(f"found current tag: {retval}")
        return retval

    def found_current_gate(self):
        """
        Have we found all the goals we set out to?
        """
        if self.goal.type != AutonomousGoal.GOAL_TYPE_GATE:
            return False
        retval = np.all([(len(self.ar_tag_poses[_id]) > 0) for _id in self.goal.tag_ids])
        self.get_logger().debug(f"found current gate: {retval}")
        return retval

    def found_tag(self, tag_id: int):
        """
        :param tag_id: the id of the tag we want to query
        :return: True if we have recorded one instance of this tag, false otherwise
        """
        assert tag_id <= ArTagManager.max_tag_id
        return len(self.ar_tag_poses[tag_id]) > 0

    def update_tags(self, msg: AlvarMarkers, depth_cam_transform: TransformStamped):
        """
        :param msg: AlvarMarker msg type, received from the ar_tag_topic
        """
        tag: AlvarMarker
        for tag in msg.markers:
            if tag.tag_id in self.ar_tag_poses:
                tag_pose_map_frame : Pose = transform.transform_pose(tag.pose.pose, depth_cam_transform.transform)
                self.ar_tag_poses[tag.tag_id].append(np.array([tag_pose_map_frame.position.x, tag_pose_map_frame.position.y]))
                self.get_logger().debug(f"found AR tag {tag.tag_id} at pose {self.ar_tag_poses[tag.tag_id]}")
