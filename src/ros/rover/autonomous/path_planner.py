#!/usr/bin/env python3
import time
from a_star import a_star
from path_2d_show import show_path 

# NOTE should probably call these something else since they are not only used by controller
from controller_params import *
from controller_math import *

from rclpy.node import Node
from core.msg import Waypoints, Waypoint, RoverPose

"""
Author: Aidan Pritchard, Max Tory and Liam Whittle

Purpose: To perform A* path planning and string pulling on 2-d grid maps. 

Subscribed topic/s: 

Published topic/s:

Services:

"""


class PathPlanner(Node):
    def __init__(self, dest, resolution_m):
        super().__init__("path_planner_node")
        
        # way-point publisher publishes a bunch of waypoints at once (hence using the 2D map dataype
        self.waypt_publisher = self.create_publisher(Waypoints, "autonomous/goals", 10)
        
        self.pose_subscriber = self.create_subscription(RoverPose, "autonomous/pose", self.update_pose, 10)

        self.resolution = resolution_m

        # in case we want to test path planning without a controller
        self.state = State()

        self.start = (0, 0)
        self.goal = dest

        self.route = []

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
        self.state.y = msg.y
        self.state.yaw = msg.yaw
        self.state.velocity = msg.velocity
        self.state.angular_velocity = msg.angular_velocity

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
        self.widt = _map.shape[1]

        self.length_meters = int(_map.shape[0] * self.resolution)
        self.width_meters = int(_map.shape[1] * self.resolution)
        
        print("Running A*")
        
        print("start: " + str(self.start))
        print("goal: " + str(self.goal))
        
        print(self.get_grid_coord(self.start))
        print(self.get_grid_coord(self.goal))
        
        print(_map.shape)

        self.route = np.array(a_star(_map, self.get_grid_coord(self.start), self.get_grid_coord(self.goal), self.resolution))

        route_coordinates = self.get_local_coords_route(self.route)
        show_path(_map, self.route, self.start, self.goal)
        waypoints = Waypoints()

        for wpt in route_coordinates:
            # publishing waypoints in order 
            waypoint = Waypoint()
            waypoint.x = wpt[0]
            waypoint.y = wpt[1]
            
            if (not math.isnan(waypoint.x)) and (not math.isnan(waypoint.y)):
                waypoints.waypoints.append(waypoint)

        print("publishing waypoints: " + str(waypoints))
        self.waypt_publisher.publish(waypoints)

