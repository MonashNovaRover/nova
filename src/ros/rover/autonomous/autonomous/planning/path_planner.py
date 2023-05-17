#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: To perform A* path planning and string pulling on 2-d grid maps. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: path_planner_node 
TOPICS:
  - subscriber: /rover/pose [RoverPose]
  - subscriber: /autonomous/ar_tag [AlvarMarkers]
  - publisher: /autonomous/waypoints [Waypoints]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	autonomous
AUTHOR(S):	Pritchard, Max Tory and Liam Whittle
CREATION:	08/03/2022
EDITED:		08/03/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - a lot 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
# nova imports
from autonomous.config.ros_config import planning_destination_topic, auto_waypoints_topic
from autonomous.config.runtime_params import INITIAL_PADDING_DIST_M, unseen_map_cost
from autonomous.math_utils import transform
from autonomous.mapping.grid_2d import Grid2D
from a_star import a_star

# ros imports
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.task import Future
from tf2_ros import Buffer, TransformListener

# msg types
from geometry_msgs.msg import Pose2D, Pose, PoseStamped
from nav_msgs.msg import Path, OccupancyGrid

# python imports
import numpy as np
import math
import time, logging


class PathPlanner(Node):
    # Status enum for A* return values
    A_STAR_SUCCESS = 0
    A_STAR_START_OBSTACLE = 1
    A_STAR_DEST_OBSTACLE = 2
    A_STAR_NO_PATH = 4
    A_STAR_CRITICAL_NO_PATH = 8

    def __init__(self):
        """
        :param resolution_m: planning resolution
        """
        super().__init__("path_planner_node")
        self.get_logger().set_level(logging.INFO)
        # constants
        self.padding_dist_m = INITIAL_PADDING_DIST_M
        self.resolution = self.declare_parameter("resolution_m", 0.1).value

        # state
        self.pose_2d = Pose2D()
        self.start = (0, 0)
        self.goal = Pose()
        self.goal_id = 0
        self.path = Path()
        self.path.header.frame_id = "local_map"

        self._map = None

        # subscribers and publishers (subscribers initialised once we have a transform)
        self.planning_subscriber = None
        self.map_subscriber = None
        self.path_publisher = self.create_publisher(Path, auto_waypoints_topic, 10)

        # Transform listeners
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, node=self, spin_thread=True)

        self.get_logger().info("Waiting for transform from 'map' to 'base_link'")
        self.transform_future : Future = self.tf_buffer.wait_for_transform_async('map', 'base_link', Time())
        self.transform_future.add_done_callback(self.transform_ready_callback)

    def transform_ready_callback(self, future: Future):
        try:
            future.result()
        except Exception as e:
            self.get_logger().error(f"Failed to get transform: {e}")
            self.destroy_node()
        else:
            self.get_logger().info("Received transform!")
            self.planning_subscriber = self.create_subscription(PoseStamped, planning_destination_topic, self.path_planning_sub_callback, 10)
            self.map_subscriber = self.create_subscription(OccupancyGrid, "/autonomous/occupancy_grid", self.update_map, 10)

    def update_map(self, msg):
        self.get_logger().debug("updating map")
        self._map = Grid2D.map_from_occupancy(msg)
        self._map[self._map < 0] = unseen_map_cost

    def set_goal(self, goal: PoseStamped):
        """
        Take a goal in any frame (typically global map frame), transform it into local map frame, and store
        """
        try:
            map_to_local_map = self.tf_buffer.lookup_transform("local_map", "map", Time()).transform
            self.get_logger().debug(f"transforming pose: {goal} by transform: {map_to_local_map}")
            local_goal : Pose = transform.transform_pose(goal.pose, map_to_local_map)
        except Exception as e:
            self.get_logger().warn(f"Failed to transform local goal: {e}")
        else:
            self.goal = local_goal

    def path_planning_sub_callback(self, msg: PoseStamped):
        """
        For path planning asynchronously from control loop without services
        """
        if self._map is None:
            self.get_logger().warn("PathPlanner: map has not been updated yet, plan could not be planned")
            return 
        self.get_logger().debug("Path planner sub callback!")
        self.set_goal(goal=msg)
        try:
            self.update_pose()
        except Exception as e:
            self.get_logger().debug(f"Transform lookup error: {e}")
        self.get_path()
        self.path_publisher.publish(self.path)

    def get_grid_coord(self, position):
        return int((position[0] + self.length_meters / 2) / self.resolution), \
               int((position[1] + self.width_meters / 2) / self.resolution)

    def get_float_position(self, coord):
        return coord[0] * self.resolution - self.length_meters / 2, \
               coord[1] * self.resolution - self.width_meters / 2,

    def update_pose(self): 
        """ 
        Callback function that updates the current pose of the rover from transform data
        """
        tf = self.tf_buffer.lookup_transform('local_map', 'base_link', time=Time())
        self.pose_2d.x = tf.transform.translation.x
        self.pose_2d.y = tf.transform.translation.y
        self.pose_2d.theta = transform.quat_to_euler(q=tf.transform.rotation)[2]
        self.path.header.stamp = tf.header.stamp

    def get_local_coords_route(self, route):
        """
        Turning a route in pixel coordinates into one in metric coordinates
         - Modified for new map - have to check it works
        """
        return [self.get_float_position((x, y)) for (x, y) in route]

    def handle_path_status(self, status):
        """
        Handles logging and adjusting of parameters according to the
        status returned by our c++ A* method.
        """
        if status & PathPlanner.A_STAR_START_OBSTACLE:
            self.get_logger().warn("started in obstacle")
        if status & PathPlanner.A_STAR_DEST_OBSTACLE:
            self.get_logger().warn("destination in obstacle")
        if status & PathPlanner.A_STAR_NO_PATH:
            self.get_logger().warn("couldn't find a path initially")
        if status & PathPlanner.A_STAR_CRITICAL_NO_PATH:
            self.get_logger().error("COULDN'T FIND PATH - NEAR OBSTACLE")
        if status == PathPlanner.A_STAR_SUCCESS:
            self.get_logger().debug("A* found safe path")

    def construct_path(self, waypoints) -> None:
            """
            Contstructs a ros2 path message from a list of waypoints
            """
            poses = []
            for waypoint in waypoints:
                if math.isnan(waypoint[0]) or math.isnan(waypoint[1]):
                    continue
                pose_stamped = PoseStamped()
                pose_stamped.header = self.path.header
                # Orient vertical
                pose_stamped.pose.orientation.w = 1.0
                pose_stamped.pose.position.x, pose_stamped.pose.position.y, pose_stamped.pose.position.z = waypoint[0], waypoint[1], 0.0

                poses.append(pose_stamped)
            self.path.poses = poses

    def get_path(self, padding=None) -> Path:
        """
        Repeatedly run A* on the updated rover pose and map to continually redetermine the optimal path.
        """
        if padding == None:
            padding = self.padding_dist_m
        if padding <= 0.4:
            return Path()

        self.start = self.pose_2d.x, self.pose_2d.y
        local_goal = self.goal.position.x, self.goal.position.y
        self.get_logger().debug(f"planning to {(local_goal[0], local_goal[1])}")

        self.length = self._map.shape[0]
        self.width = self._map.shape[1]

        self.length_meters = int(self._map.shape[0] * self.resolution)
        self.width_meters = int(self._map.shape[1] * self.resolution)
        
        t1 = time.perf_counter()
        route = np.array(a_star(self._map, self.get_grid_coord(self.start), self.get_grid_coord(local_goal), self.resolution, self.padding_dist_m))
        status = route[-1][0]
        route = route[:-1]
        self.get_logger().debug(f"planned with status {status}")
        self.get_logger().debug(f"took {time.perf_counter() - t1} s")
        self.handle_path_status(status)

        route_coordinates = np.array(self.get_local_coords_route(route))

        self.construct_path(route_coordinates)

        self.get_logger().debug(f"Path Planner Calculated {len(route_coordinates)} waypoints")
        if status & PathPlanner.A_STAR_CRITICAL_NO_PATH:
            self.get_path(padding-0.1)


def main(args=None):
    rclpy.init(args=args)
    path_planner = PathPlanner()
    rclpy.spin(path_planner)
    path_planner.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()