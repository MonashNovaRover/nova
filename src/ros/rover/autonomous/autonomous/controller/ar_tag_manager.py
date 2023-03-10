
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

from core.msg import AlvarMarker
from rclpy.node import Node
from autonomous.config.runtime_params import *
from autonomous.config.ros_config import *
from autonomous.math_utils.controller_math import Pose2D


class ArTagManager(Node):
    """
    Note: this is only a node so it can use a ROS logger
    Manages the state of AR tags. Keeps track of the last 10 AR tag global pose
    """

    max_tag_id = 5
    queue_size = 10

    def __init__(self):
        super().__init__('autonomous_ar_tag_manager')

        # These are the ids of the AR tags we want to go to
        self.ar_tag_goals = []  # if None, go to coord, if one, go to tag, if two, go through gate

        # global poses of the AR tags. We need to keep track of them as we update them since the pose at the
        # time of record is import
        self.ar_tag_poses = {i: [] for i in range(ArTagManager.max_tag_id + 1)}

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

        self.get_logger().debug(f"gate_l = {gate_l}, gate_r = {gate_r}")
        normal = np.array((gate_l[1] - gate_r[1], gate_r[0] - gate_l[0]))
        self.get_logger().debug(f"normal = {normal}")
        unit_normal = normal / np.linalg.norm(normal)
        return unit_normal

    def found_current_goals(self):
        """
        Have we found all the goals we set out to?
        """
        # return np.all([self.found_tag(goal) for goal in self.ar_tag_goals])
        found = len(self.ar_tag_goals) and np.all([self.found_tag(goal) for goal in self.ar_tag_goals])
        self.get_logger().debug("found all current goals: " + "yes" if found else "no")
        return found

    def found_tag(self, tag_id: int):
        """
        :param tag_id: the id of the tag we want to query
        :return: True if we have recorded one instance of this tag, false otherwise
        """
        assert tag_id <= ArTagManager.max_tag_id
        return len(self.ar_tag_poses[tag_id]) > 0

    def update_tags(self, msg: AlvarMarker, state: Pose2D, logger):
        """
        Given a new AR tag and rover state, update internal AR tag lists
        :param msg: AlvarMarker message type
        :param state: State of the Rover when the AR tag was recorded
        :return: None
        """

        # maintain a fixed size queue of the most recent 10 global poses
        pose = msg.pose.pose.position
        local_pose = np.array([pose.x, pose.y])

        # tracking cam extrinsics are included in global pose as 0, 0 is the centre of the rover
        extrinsics = np.array(tracking_camera_extrinsics)[:2]
        local_pose -= extrinsics

        # distance from centre of camera to ar tag
        dist = (np.dot(local_pose, local_pose)) ** 0.5

        # check it is an AR tag we care about or could care about in the future, and that the
        # distance to the camera is within our defined bounds
        if msg.id > ArTagManager.max_tag_id or (not (min_ar_distance <= dist <= max_ar_distance)):
            return

        # translate step
        rot_mat = np.array(
            [[np.cos(state.yaw), -np.sin(state.yaw)], [np.sin(state.yaw), np.cos(state.yaw)]])
        local_pose.reshape(2, 1)

        global_pose = np.matmul(rot_mat, local_pose).reshape(2) + np.array([state.x, state.y])

        logger.info("found tag: x=" + str(global_pose[0]) + " | y=" + str(global_pose[1]))

        # maintain a fixed size queue of poses
        if len(self.ar_tag_poses[msg.id]) < ArTagManager.queue_size:
            self.ar_tag_poses[msg.id] = self.ar_tag_poses[msg.id][1:]
        self.ar_tag_poses[msg.id].append((global_pose[0], global_pose[1]))

