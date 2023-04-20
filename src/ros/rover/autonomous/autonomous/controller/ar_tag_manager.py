
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
from typing import List

from core.msg import AlvarMarkers, AutonomousGoal, AlvarMarker
from rclpy.node import Node
from config.runtime_params import *
from config.ros_config import *
from math_utils.controller_math import Pose2D
from collections import deque


class ArTagManager(Node):
    """
    Note: this is only a node so it can use a ROS logger
    Manages the state of AR tags. Keeps track of the last 10 AR tag global pose
    """

    def __init__(self, max_tag_id=5, queue_size=10):
        super().__init__('autonomous_ar_tag_manager')
        self.max_tag_id = max_tag_id
        self.queue_size = queue_size

        self.goal : AutonomousGoal = None
        self.ar_tag_poses : dict = dict()

        self.sub_ar_tags = self.create_subscription(AlvarMarkers, "/ar_tracker/tags", self.callback_ar_tag, 10)

    def set_goal(self, goal: AutonomousGoal):
        """
        Provides a new goal to the AR tag manager
        """
        for _id in goal.tag_ids:
            assert(0 <= _id <= self.max_tag_id, "AR tag id out of range")

        self.goal = goal
        self.ar_tag_poses = {
            _id: deque([], maxlen=self.queue_size) for _id in goal.tag_ids
        }

    def num_tags_found(self) -> int:
        """
        :return: the number of tags we have found out of the ones we are currently searching for
        """
        return len([1 for _id in self.ar_tag_goals if len(self.ar_tag_poses[_id]) > 0])

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
        assert len(self.ar_tag_goals) == 1 or len(self.ar_tag_goals) == 2
        average_tags = [self.get_average_tag_pose(_id) for _id in self.ar_tag_goals]
        return np.mean(average_tags, axis=0)

    def get_gate_normal(self):
        """
        Assuming we have 2 tags, what is the unit vector normal to the line between them
        """
        assert len(self.ar_tag_goals) == 2
        gate_l, gate_r = self.get_average_tag_pose(self.ar_tag_goals[0]),\
                            self.get_average_tag_pose(self.ar_tag_goals[1])

        print(f"gate_l = {gate_l}, gate_r = {gate_r}")
        normal = np.array((gate_l[1] - gate_r[1], gate_r[0] - gate_l[0]))
        print(f"normal = {normal}")
        unit_normal = normal / np.linalg.norm(normal)
        return unit_normal

    def found_current_tag(self):
        """
        Were we looking for a single AR tag, and if so, have we found it?
        """
        if self.goal.type != AutonomousGoal.GOAL_TYPE_TAG:
            return False
        return len(self.ar_tag_poses[self.goal.tag_ids[0]]) > 0

    def found_current_gate(self):
        """
        Have we found all the goals we set out to?
        """
        if self.goal.type != AutonomousGoal.GOAL_TYPE_GATE:
            return False
        return np.all([(len(self.ar_tag_poses[_id]) > 0) for _id in self.goal.tag_ids])

    def found_tag(self, tag_id: int):
        """
        :param tag_id: the id of the tag we want to query
        :return: True if we have recorded one instance of this tag, false otherwise
        """
        assert tag_id <= ArTagManager.max_tag_id
        return len(self.ar_tag_poses[tag_id]) > 0

    def callback_ar_tag(self, msg: AlvarMarkers):
        """
        :param msg: AlvarMarker msg type, received from the ar_tag_topic
        """
        tag: AlvarMarker
        for tag in msg.markers:
            if tag.tag_id in self.ar_tag_poses:
                self.ar_tag_poses[tag.tag_id].append(tag.pose.pose.position)