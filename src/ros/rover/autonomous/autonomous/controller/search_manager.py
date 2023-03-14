#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Manages a search plan, taking an initial
            search area and a search pattern, and
            listening to detected blocks and AR 
            tags to dynamically update the
            rover's planning goals
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: search_manager
TOPICS:
  - subscriber: /object_detector/markers [MarkerArray]
  - subscriber: /ar_tracker/tags [MarkerArray]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    autonomous
AUTHOR(S):	Max Tory
CREATION:	14/03/2023
EDITED:		14/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
  - Make it work
  - testestestestestestestestest
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
# ros imports
from typing import Union
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from tf2_ros import Buffer, TransformListener

# msg types
from core.msg import AlvarMarker, AlvarMarkers, AutonomousGoal
from visualization_msgs.msg import MarkerArray, Marker
from geometry_msgs.msg import PoseStamped

# nova imports
import autonomous.math_utils.transform as transform

# standard python imports
import numpy as np

SQRT_2 = np.sqrt(2)
SQRT_3 = np.sqrt(2)

# Normalised RGB values for each block colour. The dot product of these with the
# camera's RGB values will give us a measure of how close the colour is to each colour option
COLOR_VECTORS = {
    "RED": np.array([1., 0., 0.]),
    "GREEN": np.array([0., 1., 0.]),
    "BLUE": np.array([0., 0., 1.]),
    "YELLOW": np.array([1., 1., 0.]) / SQRT_2,
    "WHITE": np.array([1., 1., 1.]) / SQRT_3
}


class SearchManager(Node):
    """
    Manages the search of the map for blocks and AR tags. Returns goals to the controller, based on a predetermined
    search plan, as well as dynamically updating the search plan based on detected blocks and AR tags. Each newly
    detected block or AR tag wiill initially be stored as "unsure", and will only be added to the search plan once its
    location has been repeatedly confirmed. Once a block or AR tag has been found, it will be removed from the search.
    """
    MIN_SAMPLES=10
    MAX_STD_DEV=0.2

    def __init__(self):
        super().__init__("search_manager")
        # ROS Subscribers
        self.sub_blocks = self.create_subscription(MarkerArray, "/object_detector/markers", self.cb_cube, 10)
        self.sub_tags = self.create_subscription(AlvarMarkers, "/tracking_camera/tags", self.cb_tag, 10)

        # ROS Parameters
        self.param_search_plan = self.declare_parameter("search_plan", []).value
        self.param_desired_tags = self.declare_parameter("tracked_tag_ids", []).value
        self.param_desired_blocks = self.declare_parameter("tracked_block_colors", []).value

        # ROS Tf2 stuff
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(buffer=self.tf_buffer, node=self, spin_thread=True)

        self.goals = []
        self.active_goal = None
        self.init_goals

        # Internal variables
        self.last_tags : AlvarMarkers= None
        self.new_tags = False
        self.last_blocks : MarkerArray = None
        self.new_blocks = False

        self.found_tags = dict()
        self.found_blocks = dict()

        self.unsure_tags = {_id: [] for _id in self.param_desired_tags}
        self.unsure_blocks = {color: [] for color in self.param_desired_blocks}
        
        timer_period = 0.1  # run the timer 10 times per second
        self.create_timer(timer_period, self.handle_targets)


    def init_goals(self):
        """
        Initialises the search goals based on the map bounds and the search pattern. Adds a spin on each corner of the map to look
        for targets
        """
        for x, y in np.array(self.param_map_bounds).reshape(-1, 2):
            goal = AutonomousGoal()
            goal.position.x, goal.position.y = x, y
            goal.type = AutonomousGoal.GOAL_TYPE_HONING

            spin_goal = AutonomousGoal()
            spin_goal.position.x, spin_goal.position.y = x, y
            spin_goal.type = AutonomousGoal.GOAL_TYPE_SPIN
            
            self.goals.append(goal)
            self.goals.append(spin_goal)

        self.active_goal = self.goals.pop(0)

    def cb_cube(self, msg : MarkerArray):
        """
        Callback for the /object_detector/markers topic. Receives a MarkerArray of all detected blocks.
        """
        self.last_blocks = msg
        if len(msg.markers) > 0:
            self.new_blocks = True

    def cb_tag(self, msg : AlvarMarkers):
        """
        Callback for the /ar_tracker/tags topic. Receives a MarkerArray of all detected AR tags.
        """
        self.last_tags = msg
        if len(msg.markers) > 0:
            self.new_tags = True

    def get_closest_color(self, block: Marker) -> str:
        """
        Use the dot product to compare the block's colour to the known block colours. The maximum dot
        product value will be the closest colour.
        """
        block_color = np.array([block.color.r, block.color.g, block.color.b])
        max_dot, max_color = 0, None
        for color in COLOR_VECTORS:
            dot = np.dot(block_color, COLOR_VECTORS[color])
            if dot > max_dot:
                max_dot = dot
                max_color = color

        self.get_logger().debug(f"Closest color to {block_color} is {max_color} with dot product {max_dot}")
        return max_color

    def to_local_map(self, msg: Union(PoseStamped, Marker)):
        """
        Converts a PoseStamped from the camera frame to the local map frame.
        """
        try: 
            local_map_transform = self.tf_buffer.lookup_transform(msg.header.frame_id, "local_map", Time.from_msg(msg.header.stamp)).transform
            local_map_pose = transform.transform_pose(msg.pose, local_map_transform)
            return local_map_pose
        except:
            self.get_logger().warn("Could not find transform from camera to local map")
            return None

    def remove_outlier_pos(self, pos_vals):
        """
        Removes any outlier positions from the list of positions. An outlier is defined as a position that is
        more than 3 standard deviations away from the mean.
        """
        mean = np.mean(pos_vals, axis=0)
        std_dev = np.std(pos_vals, axis=0)

        return [pos for pos in pos_vals if np.all(np.abs(pos - mean) < 2 * std_dev)]

    def attempt_confirm_target(self, _id=None, color=None):
        """
        Checks that we have enough samples of this tag id, and that their position is sufficiently consistent
        to be considered a confirmed tag.
        """
        target_pos = self.unsure_tags[_id] if _id is not None else self.unsure_blocks[color]
        consistent_pos = self.remove_outlier_pos(target_pos)

        if len(consistent_pos) >= SearchManager.MIN_SAMPLES:
            target_pos_vals = consistent_pos[-SearchManager.MIN_SAMPLES:]
            # We have enough samples to be confident in this tag's position
            # Calculate the average position of the tag
            avg_pos = np.mean(target_pos_vals, axis=0)
            # Calculate the standard deviation of the tag's position
            std_dev = np.std(target_pos_vals, axis=0)
            # Check that the standard deviation is small enough to be considered a confirmed tag
            if np.all(std_dev < SearchManager.MAX_STD_DEV):
                # We have a confirmed tag
                if _id is not None:
                    self.found_tags[_id] = avg_pos
                    self.unsure_tags.pop(_id)
                if color is not None:
                    self.found_blocks[color] = avg_pos
                    self.unsure_blocks.pop(color)

    def update_tags(self):
        """
        Updates the list of detected tags, adding any new tags to the search plan and removing any tags that have been
        found.
        """
        for tag in self.last_tags.markers:
            _id = tag.tag_id
            if _id in self.found_tags or _id not in self.param_desired_tags:
                # We already know where the tag is, or we don't care about this tag
                continue
            else:
                # We care about this tag and haven't worked out where it is
                tag_pose_stamped = tag.pose
                local_map_pose = self.to_local_map(tag_pose_stamped)
                if local_map_pose is None:
                    continue
                # Append the tag's pose to the list of estimated poses
                self.unsure_tags[_id].append([local_map_pose.position.x, local_map_pose.position.y])
                self.attempt_confirm_target(_id=_id)

        self.new_tags = False

    def update_blocks(self):
        """
        Updates the list of detected blocks, adding any new blocks to the search plan and removing any blocks that have been
        found.
        """
        for block in self.last_blocks.markers:
            color = self.get_closest_color(block)
            if color in self.found_blocks or color not in self.param_desired_blocks:
                # We already know where the block is, or we don't care about this block
                continue
            else:
                # We care about this block and haven't worked out where it is
                local_map_pose = self.to_local_map(block)
                if local_map_pose is None:
                    continue
                # Append the block's pose to the list of estimated poses
                self.unsure_blocks[color].append([local_map_pose.position.x, local_map_pose.position.y])
                self.attempt_confirm_target(color=color)

        self.new_blocks = False

    def search_complete(self):
        """
        Returns true if we have found all the blocks and tags we are looking for
        """
        return len(self.found_blocks) == len(self.param_desired_blocks) \
            and len(self.found_tags) == len(self.param_desired_tags) \
            and len(self.goals) == 0 \
            and self.active_goal is None

    def at_goal(self):
        """
        Called when the robot has reached its current goal
        """
        if self.active_goal.type in [AutonomousGoal.GOAL_TYPE_HONING, AutonomousGoal.GOAL_TYPE_SPIN]:
            self.active_goal = self.goals.pop(0)

    def get_nearest_block_or_tag(self, rover_pos):
        """
        Returns the nearest block or tag to the rover which we are unsure about. Returns None if there are
        not tags we are unsure about
        """
        nearest_target = None
        nearest_target_dist = float("inf")
        for color in self.unsure_blocks:
            target_pos = np.mean(self.unsure_blocks[color], axis=0)
            dist = np.linalg.norm(target_pos - np.array(rover_pos))
            if dist < nearest_target_dist:
                nearest_target = AutonomousGoal()
                nearest_target.type = AutonomousGoal.GOAL_TYPE_BLOCK
                nearest_target.block_color = color
                nearest_target.position.x, nearest_target.position.y = target_pos[0], target_pos[1]
                nearest_target_dist = dist
        for _id in self.unsure_tags:
            target_pos = np.mean(self.unsure_tags[_id], axis=0)
            dist = np.linalg.norm(target_pos - rover_pos)
            if dist < nearest_target_dist:
                nearest_target = AutonomousGoal()
                nearest_target.type = AutonomousGoal.GOAL_TYPE_TAG
                nearest_target.tag_id = _id
                nearest_target.position.x, nearest_target.position.y = target_pos[0], target_pos[1]
                nearest_target_dist = dist

        return nearest_target, nearest_target_dist
        

    def get_current_goal(self, current_pos):
        """
        Returns the next goal to navigate to
        """

        if self.active_goal.type == AutonomousGoal.GOAL_TYPE_BLOCK:
            if self.active_goal.color in self.found_blocks:
                # We have found this block, so we can remove it from the search plan
                self.active_goal = self.goals.pop(0)
        elif self.active_goal.type == AutonomousGoal.GOAL_TYPE_TAG:
            if self.active_goal.tag_id in self.found_tags:
                # We have found this tag, so we can remove it from the search plan
                self.active_goal = self.goals.pop(0)
        elif self.active_goal.type == AutonomousGoal.GOAL_TYPE_HONING:
            if len(self.unsure_blocks) > 0 or len(self.unsure_tags) > 0:
                nearest_target, nearest_target_dist = self.get_nearest_block_or_tag(current_pos)
                if nearest_target_dist < np.linalg.norm([[current_pos[0] - self.active_goal.position.x],
                                                         [current_pos[1] - self.active_goal.position.y]]):
                    self.goals = [self.active_goal] + self.goals
                    self.active_goal = nearest_target
        
        return self.active_goal



    def handle_targets(self):
        if self.new_blocks:
            self.update_blocks()
        if self.new_tags:
            self.update_tags()



def main():
    rclpy.init()
    node = SearchManager()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
