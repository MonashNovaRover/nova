#!/usr/bin/env python3
_package__ = "autonomous"

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


from a_star import a_star
from math_utils.controller_math import *
from rclpy.node import Node
from core.msg import Waypoints, Waypoint, RoverPose, AlvarMarker
from config.ros_config import *
from config.runtime_params import min_ar_distance, max_ar_distance

class PathPlanner(Node):
    # Status enum for A* return values
    A_STAR_SUCCESS = 0
    A_STAR_START_OBSTACLE = 1
    A_STAR_DEST_OBSTACLE = 2
    A_STAR_NO_PATH = 4
    A_STAR_CRITICAL_NO_PATH = 8

    def __init__(self, dest: list, resolution_m):
        super().__init__("path_planner_node")
        
        # way-point publisher publishes a bunch of waypoints at once (hence using the 2D map datatype
        self.waypt_publisher = self.create_publisher(Waypoints, auto_waypoints_topic, 10)
        
        self.pose_subscriber = self.create_subscription(RoverPose, rover_pose_topic, self.update_pose, 10)

        self.ar_tag_subscriber = self.create_subscription(AlvarMarker, "autonomous/ar_tag", self.update_goal_callback, 10)

        self.expected_goal_id = 0

        self.resolution = resolution_m

        self.recorded_tags = []

        # in case we want to test path planning without a controller
        self.state = State()

        self.start = (0, 0)
        self.goal = dest

        self.route = []

    def update_goal_callback(self, msg):
        """
        1. Check that it's the AR tag we are looking
        2. Filter out dodgy values
            - are values within an absolute range?
            - standard deviation? idk
        3. Transform pose of tag relative to rover into global pose of tag
        
        
        """
        print("~~~~~~~~~~~~~~~~~~~~~~~~~~")
        print(msg.pose)
        pose = msg.pose.pose.position

        # filter step
        distance = (pose.z ** 2 + pose.y ** 2) ** 0.5
        # if not (min_ar_distance <= distance <= max_ar_distance):
        #    return

        # translate step
        global_pose_x = pose.x * np.cos(self.state.yaw) - pose.y * np.sin(self.state.yaw) + self.state.x
        global_pose_y = pose.x * np.sin(self.state.yaw) + pose.y * np.cos(self.state.yaw) + self.state.y

        # goal diff for logging
        print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
        print(str(self.goal))
        goal_diff = ((self.goal[0] - global_pose_x) ** 2 + (self.goal[1] - global_pose_y) ** 2) ** 0.5

        # self.get_logger().info("Updated planning goal: ")

        self.goal = global_pose_x, global_pose_y

    def get_grid_coord(self, position):
        return int((position[0] + self.length_meters / 2) / self.resolution), \
               int((position[1] + self.width_meters / 2) / self.resolution)

    def get_float_position(self, coord):
        return coord[0] * self.resolution - self.length_meters / 2, \
               coord[1] * self.resolution - self.width_meters / 2,

    def update_pose(self, msg): 
        """ 
        Callback function that updates the current pose of the rover from data in the auto_command_pose_updates topic
        """
        self.state.x = msg.x


    @staticmethod
    def heuristic(a, b, heuristic_type="euclidean"):
       
        """
        The manhattan heuristic works for movements up, down, left, right, and is an admissible and consistent heuristic
        The straight line heuristic works for any problem domain in 2-d euclidean space and is admissible and consistent
        The octile heuristic is admissible and consistent for octile movements where diagonal movements incur cost sqrt(2)
        """
        if heuristic_type == "manhattan":
            return abs(a[0] - b[0]) + abs(a[1] - b[1])  # manhattan
        elif heuristic_type == "octile":
            return min(abs(a[0] - b[0]), abs(a[1] - b[1])) * (2 ** 0.5) + abs(abs(a[0] - b[0]) - abs(a[1] - b[1]))  # octile
        return np.sqrt((b[0] - a[0]) ** 2 + (b[1] - a[1]) ** 2)  # euclidean distance norm

    def get_local_coords_route(self, route):
        """
        Turning a route in pixel coordinates into one in metric coordinates
         - Modified for new map - have to check it works
        """
        return [self.get_float_position((x, y)) for (x, y) in route]

    def get_path(self, _map):
        """
        Repeatedly run A* on the updated rover pose and map to continually redetermine the optimal path.
        Called on a clock initialised in the add_destination method
        """

        self.start = (self.state.x, self.state.y)
       
        self.length = _map.shape[0]
        self.width = _map.shape[1]

        self.length_meters = int(_map.shape[0] * self.resolution)
        self.width_meters = int(_map.shape[1] * self.resolution)
        print(self.length_meters)
        print("Running A* for goal: " + str(self.goal))
        
        self.route = np.array(a_star(_map, self.get_grid_coord(self.start), self.get_grid_coord(self.goal), self.resolution))
        status = self.route[-1, 0]
        self.route = self.route[:-1]
        if status & PathPlanner.A_STAR_START_OBSTACLE: self.get_logger().warn("started in obstacle")
        if status & PathPlanner.A_STAR_DEST_OBSTACLE: self.get_logger().warn("dest in obstacle")
        if status & PathPlanner.A_STAR_NO_PATH: self.get_logger().warn("couldn't find a path initially")
        if status & PathPlanner.A_STAR_CRITICAL_NO_PATH: self.get_logger().error("COULDN'T FIND PATH - NEAR OBSTACLE")
        if status == PathPlanner.A_STAR_SUCCESS: self.get_logger().info("A* found safe path")
        route_coordinates = self.get_local_coords_route(self.route)
        waypoints = Waypoints()

        for wpt in route_coordinates:
            # publishing waypoints in order 
            waypoint = Waypoint()
            waypoint.x = wpt[0]
            waypoint.y = wpt[1]
           
            if (not math.isnan(waypoint.x)) and (not math.isnan(waypoint.y)):
                waypoints.waypoints.append(waypoint)

        self.waypt_publisher.publish(waypoints)

