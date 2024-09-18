#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Manages blocks detected with a custom
    model on our depth camera. Store the
    detections in a queue and report a confirmed
    pose once we have enough readings with a
    sufficiently low standard deviation.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: cube_tracker
TOPICS:
  - publisher: /TBD [MarkerArray]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_cube_localisation
AUTHOR(S):	Max Tory
CREATION:	14/03/2023
EDITED:		08/03/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
  - Input initial map coords
  - New model/training data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# standard python imports
import logging
from typing import Dict, List, Union
import numpy as np

from rclpy.node import Node
from rclpy.time import Time
from tf2_ros import Buffer, TransformListener

# msg types
from visualization_msgs.msg import MarkerArray, Marker
from geometry_msgs.msg import PoseStamped, Pose, Transform, Pose2D
from std_msgs.msg import ColorRGBA

# ros imports
import rclpy

# Hue ranges for the different block colours except white. Any hue can be white if the lightness is high enough


IDEAL_VECTORS = {
    "RED": [1.0, 0.0, 0.0],
    "GREEN": [0.0, 1.0, 0.0],
    "BLUE": [0.0, 0.0, 1.0],
    "WHITE": [1.0, 1.0, 1.0],
}

IDS_COLOR = {
    0: "RED",
    1: "GREEN",
    2: "BLUE",
    3: "WHITE",
}

COLOR_IDS = {
    "RED": 0,
    "GREEN": 1,
    "BLUE": 2,
    "WHITE": 3,
}


# TODO: Change this to only track cubes, not AR tags, and not worry about search plan
class CubeTracker(Node):
    """
    Manages the search of the map for blocks. Each newly detected block will initially be stored as "unsure", 
    and will only be localised once its location has been repeatedly confirmed. Once a block 
    has been found, it will be ignored.
    """
    MIN_SAMPLES = 5
    MAX_STD_DEV = 0.2

    def __init__(self):
        super().__init__("cube_tracker")
        self.get_logger().set_level(logging.DEBUG)
        # ROS Subscribers
        self.sub_blocks = self.create_subscription(
            MarkerArray, "/oak/nn/spatial_detections_markers", self.cb_cube, 10)

        # ROS publishers
        self.pub_confirmed_targets = self.create_publisher(
            MarkerArray, "~/confirmed_cubes", 10)

        # ROS Parameters
        # (to remove) self.param_desired_blocks = self.declare_parameter("tracked_block_colors", []).value
        self.param_max_reasonable_z = self.declare_parameter(
            "maximum_target_z", 1.5).value
        self.param_min_reasonable_z = self.declare_parameter(
            "minimum_target_z", -1.0).value
        self.param_map_coords_counterclockwise = self.declare_parameter(
            "map_coords_cc", [10, 10, -10, 10, -10, -10, 10, -10]).value

        # (to remove) if self.param_desired_blocks is None:
        #     self.param_desired_blocks = []

        # ROS Tf2 stuff
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(
            buffer=self.tf_buffer, node=self, spin_thread=True)

        self.map_xys_3d, self.map_edges = self.get_map_edges_from_boundary_points()
        self.state_rover_pose = Pose2D()

        # Internal variables
        self.last_blocks: MarkerArray = None
        self.new_blocks = False

        self.found_blocks = dict()

        self.unsure_blocks: Dict[List] = dict()

        timer_period = 0.1  # run the timer 10 times per second
        self.create_timer(timer_period, self.handle_targets)

        self.state_rover_pose.x = 0.  # (to remove)
        self.state_rover_pose.y = 0.  # (to remove)
        self.state_rover_pose.theta = 0.  # (to remove)

    def get_map_edges_from_boundary_points(self):
        """
        Takes the map boundary points and returns a list of the edges of the map.
        Map boundary points are assumed to be provided in the counter-clockwise order.
        Returns a list of edges, where each edge is a 3D vector with zero z component, to be usable with cross product.
        """
        map_xys = np.array(
            self.param_map_coords_counterclockwise).reshape(-1, 2)
        map_xys_3d = np.hstack((map_xys, np.zeros((len(map_xys), 1))))
        map_edges = np.array(
            [map_xys_3d[(i + 1) % len(map_xys_3d)] - point for i, point in enumerate(map_xys_3d)])
        return map_xys_3d, map_edges

    def cb_cube(self, msg: MarkerArray):
        """
        Callback for the /object_detector/markers topic. Receives a MarkerArray of all detected blocks.
        """
        self.last_blocks = msg
        if len(msg.markers) > 0:
            self.new_blocks = True

    def to_map(self, msg: Union[PoseStamped, Marker]):
        """
        Converts a PoseStamped from the camera frame to the local map frame.
        """
        try:

            msg_pose_stamped = PoseStamped()
            msg_pose_stamped.header = msg.header
            msg_pose_stamped.pose = msg.pose
            # (to keep) local_map_pose = self.tf_buffer.transform(msg_pose_stamped, 'map')
            local_map_pose = msg.pose  # (to remove)
            return local_map_pose
        except Exception as e:
            self.get_logger().warn(
                f"Error translating pose to local map frame: {e}")
            return None

    def remove_outlier_pos(self, pos_vals):
        """
        Removes any outlier positions from the list of positions. An outlier is defined as a position that is
        more than 3 standard deviations away from the mean.
        """
        mean = np.mean(pos_vals, axis=0)
        std_dev = np.std(pos_vals, axis=0)

        return [pos for pos in pos_vals if np.all(np.abs(pos - mean) < 2 * std_dev)]

    def attempt_confirm_target(self, color=None):
        """
        Checks that we have enough samples of this block, and that their position is sufficiently consistent
        to be considered a confirmed block.
        """
        target_pos = self.unsure_blocks[color]
        if len(target_pos) >= CubeTracker.MIN_SAMPLES:
            consistent_pos = self.remove_outlier_pos(target_pos)
        else:
            self.get_logger().debug(
                f"{len(target_pos)} samples is not enough to confirm target {color}")
            return

        if len(consistent_pos) >= CubeTracker.MIN_SAMPLES:
            self.get_logger().debug(
                f"Validating consistency of target {color}: {consistent_pos}")
            target_pos_vals = consistent_pos[-CubeTracker.MIN_SAMPLES:]
            # We have enough samples to be confident in this block's position
            # Calculate the average position of the block
            avg_pos = np.mean(target_pos_vals, axis=0)
            # Calculate the standard deviation of the block's position
            std_dev = np.std(target_pos_vals, axis=0)
            # Check that the standard deviation is small enough to be considered a confirmed block
            if np.all(std_dev < CubeTracker.MAX_STD_DEV):
                self.get_logger().debug(
                    f"Confirmed target {color} consistent pos at {avg_pos}")
                # We have a confirmed block
                if color is not None:
                    self.found_blocks[color] = avg_pos
                    self.unsure_blocks.pop(color)
            else:
                self.get_logger().debug(
                    f"Target {color} is not consistent enough: {consistent_pos}")

    def pose_in_map(self, pose: Pose) -> bool:
        """
        Checks if a pose is in the map by taking cross products with the edges of the map, in the counter-clockwise direction.
        """
        pos_vec = np.array([pose.position.x, pose.position.y, 0])
        self.get_logger().debug(f"Checking if pos {pose.position} is in map")
        for corner, edge in zip(self.map_xys_3d, self.map_edges):
            self.get_logger().debug(
                f"corner: {corner}, edge: {edge}, pos_vec: {pos_vec}")
            if np.cross(edge, pos_vec - corner)[2] < 0:
                # negative z component of cross product means the point is clockwise from the edge. This places it outside the edge and not in the map
                # Requires counter-clockwise ordering of edges
                return False

        return True

    def pose_not_reasonable(self, pose: Pose) -> bool:
        """
        Return true if the pose of an object is not reasonable for it to be in the map
        """
        not_reasonable = False

        if pose.position.z > self.param_max_reasonable_z or pose.position.z < self.param_min_reasonable_z:
            not_reasonable = True

        elif not self.pose_in_map(pose):
            not_reasonable = True

        return not_reasonable

    def update_blocks(self):
        """
        Updates the list of detected blocks, localising any new blocks and ignoring any blocks that have been
        found.
        """
        for block in self.last_blocks.markers:
            color = IDS_COLOR[block.id]
            # (to remove) or color not in self.param_desired_blocks:
            if color in self.found_blocks:
                # We already know where the block is, or we don't care about this block
                continue
            else:
                # We care about this block and haven't worked out where it is
                local_map_pose = self.to_map(block)
                if local_map_pose is None or self.pose_not_reasonable(local_map_pose):
                    self.get_logger().debug(
                        f"pose: {local_map_pose} not reasonable")
                    continue
                # Append the block's pose to the list of estimated poses
                if color not in self.unsure_blocks:
                    self.unsure_blocks[color] = []
                self.unsure_blocks[color].append(
                    [local_map_pose.position.x, local_map_pose.position.y])
                self.attempt_confirm_target(color=color)

        self.new_blocks = False

    def callback_rover_pose(self):
        """
        Stores the latest rover pose message into our State() variable.
        """
        try:
            base_link_tf: Transform = self.tf_buffer.lookup_transform(
                "map", "base_link", Time()).transform
            self.get_logger().debug("Found transform from local_map to base_link", once=True)
        except Exception:
            self.get_logger().warn("No transform from local_map to base_link", once=True)
        else:
            self.state_rover_pose.x = base_link_tf.translation.x
            self.state_rover_pose.y = base_link_tf.translation.y
            # self.state_rover_pose.theta = transform.quat_to_euler(base_link_tf.rotation)[2]
            q = base_link_tf.rotation
            t3 = 2 * (q.w * q.z + q.x * q.y)
            t4 = 1 - 2 * (q.y * q.y + q.z * q.z)
            yaw = np.arctan2(t3, t4)
            self.state_rover_pose.theta = yaw

    def publish_found(self):
        """
        Publishes the found blocks.
        """
        array = MarkerArray()
        for color, pos in self.found_blocks.items():
            msg = self.found_block_msg(color, pos)
            array.markers.append(msg)

        self.pub_confirmed_targets.publish(array)

    def found_block_msg(self, color_name: str, pos: List[float]) -> Marker:
        """
        Finalises a block.
        """
        msg = Marker()
        pose = Pose()
        pose.position.x = pos[0]
        pose.position.y = pos[1]
        pose.position.z = 0.0
        pose.orientation.w = 1.0
        msg.pose = pose
        msg.type = Marker.CUBE
        msg.scale.x = .1
        msg.scale.y = .1
        msg.scale.z = .1
        color = ColorRGBA()
        color.r = IDEAL_VECTORS[color_name][0]
        color.g = IDEAL_VECTORS[color_name][1]
        color.b = IDEAL_VECTORS[color_name][2]
        color.a = 1.
        msg.color = color
        msg.header.frame_id = "map"
        msg.header.stamp = self.get_clock().now().to_msg()
        # Namespace - raw messages can be separated from confirmed cubes
        msg.ns = "completed"
        msg.id = COLOR_IDS[color_name]

        return msg

    def handle_targets(self):
        if self.new_blocks:
            self.update_blocks()
        self.publish_found()


def main(args=None):
    rclpy.init(args=args)
    node = CubeTracker()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
