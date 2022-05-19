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
from core.msg import Waypoints, Waypoint, RoverPose, Point2D
from config.ros_config import *
from config.runtime_params import ignore_waypoints, INITIAL_PADDING_DIST_M, goal_achieved_distance
from core.srv import PathPlanningRequest
from mapping.grid_2d import Grid2D


class PathPlanner(Node):
    # Status enum for A* return values
    A_STAR_SUCCESS = 0
    A_STAR_START_OBSTACLE = 1
    A_STAR_DEST_OBSTACLE = 2
    A_STAR_NO_PATH = 4
    A_STAR_CRITICAL_NO_PATH = 8

    def __init__(self, resolution_m):
        """
        :param resolution_m: planning resolution
        """
        super().__init__("path_planner_node")

        # constants
        self.padding_dist_m = INITIAL_PADDING_DIST_M
        self.resolution = resolution_m

        # state
        self.state = State()
        self.at_goal = True
        self.start = (0, 0)
        self.goal = (0, 0)
        self.goal_id = 0
        self.offset = [0, 0]
        self.route = []

        self.grid2d = None

        # subscribers and services
        # planning service listens to requests for paths to be planned
        self.planning_service = self.create_service(PathPlanningRequest, path_planning_service_name,
                                                    self.path_planning_service_callback)
        self.pose_subscriber = self.create_subscription(RoverPose, rover_pose_topic, self.update_pose, 10)
        self.planning_subscriber = self.create_subscription(Point2D, planning_destination_topic, self.path_planning_sub_callback, 10)
        self.path_publisher = self.create_publisher(Waypoints, auto_waypoints_topic, 10)

    def update_map(self, msg):
        self.grid2d = msg

    def path_planning_service_callback(self, request: PathPlanningRequest.Request,
                                       response: PathPlanningRequest.Response):
        """
        This callback is used in the path planning service. The service will receive a goal coordinate and return
        a set of destination coordinates, and a valid boolean response if a reasonable path could be found
        """
        if not self.grid2d:
            response.success = False
            response.path = []
            self.get_logger().warn("PathPlanner: map has not been updated yet, plan could not be planned")

        # fill the way-points
        response.path = self.get_path(request.target.x, request.target.y)
        response.success = True
        return response

    def path_planning_sub_callback(self, msg):
        """
        For path planning asynchronously from control loop without services
        """
        if not self.grid2d:
            response.success = False
            response.path = []
            self.get_logger().warn("PathPlanner: map has not been updated yet, plan could not be planned")

        path = self.get_path(msg.x, msg.y)
        self.path_publisher.publish(path)

    def set_offset(self, offset):
        self.offset[0] = offset[0]
        self.offset[1] = offset[1]

    def manual_goal_callback(self, msg):
        """
        1. Check that it's the AR tag we are looking
        2. Filter out dodgy values
            - are values within an absolute range?
            - standard deviation? idk
        3. Transform pose of tag relative to rover into global pose of tag
        """
        self.get_logger().info("Next goal x=" + str(msg.x) + " | y=" + str(msg.y))
        self.goal = (msg.x, msg.y)
        self.at_goal = False

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
        self.x = msg.x
        self.state.y = msg.y
        self.state.yaw = msg.yaw

        if not self.at_goal and distance((self.x, self.state.y), self.goal) < goal_achieved_distance:
            self.achieved_goal()

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
        if status & PathPlanner.A_STAR_START_OBSTACLE: self.get_logger().warn("started in obstacle")
        if status & PathPlanner.A_STAR_DEST_OBSTACLE: self.get_logger().warn("dest in obstacle")
        if status & PathPlanner.A_STAR_NO_PATH: self.get_logger().warn("couldn't find a path initially")
        if status & PathPlanner.A_STAR_CRITICAL_NO_PATH:
            self.get_logger().error("COULDN'T FIND PATH - NEAR OBSTACLE")
            self.padding_dist_m -= 0.1
            if self.padding_dist_m < 0.4:
                self.get_logger().error("Ah HECK")
                return
        if status == PathPlanner.A_STAR_SUCCESS: self.get_logger().info("A* found safe path")

    # todo: migrate this logic to strategy manager
    def achieved_goal(self):
        """
        Inform the operators that we think we have achieved a goal,
        and update the state of the planner accordingly.
        """
        self.at_goal = True
        self.get_logger().info(f"GOAL ACHIEVED: ({self.goal[0]}, {self.goal[1]}) [id = {self.goal_id}]")

    def get_path(self, goal_x, goal_y) -> Waypoints:
        """
        Repeatedly run A* on the updated rover pose and map to continually redetermine the optimal path.
        Called on a clock initialised in the add_destination method
        """

        # todo: this logic should also be outside the path planner - for now we can keep it here I guess
        if self.at_goal: 
            self.get_logger().info("No goal to navigate to")
            empty_waypoints = Waypoints()
            empty_waypoints.waypoints = []
            return empty_waypoints

        self.start = (self.x - self.offset[0], self.state.y - self.offset[1])
        local_goal = (goal_x - self.offset[0], goal_y - self.offset[1])

        self.length = self.grid2d.shape[0]
        self.width = self.grid2d.shape[1]

        self.length_meters = int(self.grid2d.shape[0] * self.resolution)
        self.width_meters = int(self.grid2d.shape[1] * self.resolution)
        
        self.route = np.array(a_star(self.grid2d, self.get_grid_coord(self.start), self.get_grid_coord(local_goal), self.resolution, self.padding_dist_m))
        self.route = self.route[:-1]

        route_coordinates = np.array(self.get_local_coords_route(self.route))
        route_coordinates[:] += np.array(self.offset)
        waypoints = Waypoints()

        # todo: the logic of ignoring waypoints should be outside the path planner. It should plan a pure path
        for wpt in route_coordinates[min(len(route_coordinates) - 1, ignore_waypoints):]:
            waypoint = Waypoint()
            waypoint.x = wpt[0]
            waypoint.y = wpt[1]
           
            if (not math.isnan(waypoint.x)) and (not math.isnan(waypoint.y)):
                waypoints.waypoints.append(waypoint)

        self.get_logger().info(f"Path Planner Calculated {len(route_coordinates)} waypoints")
        return waypoints


def handle_path_status(status: int):
        """
        Handles logging and adjusting of parameters according to the
        status returned by our c++ A* method.
        """    
        if status & PathPlanner.A_STAR_START_OBSTACLE: self.get_logger().warn("started in obstacle")
        if status & PathPlanner.A_STAR_DEST_OBSTACLE: self.get_logger().warn("dest in obstacle")
        if status & PathPlanner.A_STAR_NO_PATH: self.get_logger().warn("couldn't find a path initially")
        if status & PathPlanner.A_STAR_CRITICAL_NO_PATH:
            self.get_logger().error("COULDN'T FIND PATH - NEAR OBSTACLE")
            self.padding_dist_m -= 0.1
            if self.padding_dist_m < 0.4:
                self.get_logger().error("Ah HECK")
                return
        if status == PathPlanner.A_STAR_SUCCESS: self.get_logger().info("A* found safe path")


