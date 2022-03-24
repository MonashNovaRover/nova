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
from core.msg import Waypoints, Waypoint, RoverPose, AlvarMarker, AutonomousGoal
from config.ros_config import *
from config.runtime_params import min_ar_distance, max_ar_distance, ignore_waypoints, tracking_camera_extrinsics, INITIAL_PADDING_DIST_M, goal_achieved_distance
from nav_msgs.msg import Odometry    


class PathPlanner(Node):
    # Status enum for A* return values
    A_STAR_SUCCESS = 0
    A_STAR_START_OBSTACLE = 1
    A_STAR_DEST_OBSTACLE = 2
    A_STAR_NO_PATH = 4
    A_STAR_CRITICAL_NO_PATH = 8

    def __init__(self, resolution_m):
        super().__init__("path_planner_node")
        
        # way-point publisher publishes a bunch of waypoints at once (hence using the 2D map datatype
        self.waypt_publisher = self.create_publisher(Waypoints, auto_waypoints_topic, 10)
        self.padding_dist_m = INITIAL_PADDING_DIST_M
        self.alvar_publisher = self.create_publisher(Odometry, "autonomous/ar_tag/global_odom", 10)
        
        self.pose_subscriber = self.create_subscription(RoverPose, rover_pose_topic, self.update_pose, 10)

        self.ar_tag_subscriber = self.create_subscription(AlvarMarker, ar_track_topic, self.ar_goal_callback, 10)
        self.goal_subscriber = self.create_subscription(AutonomousGoal, auto_goals_topic, self.manual_goal_callback, 10)

        self.expected_goal_id = 0

        self.resolution = resolution_m

        self.recorded_tags = []

        # in case we want to test path planning without a controller
        self.state = State()

        # need to provide a goal before we start planning
        self.at_goal = True

        self.start = (0, 0)
        self.goal = (0, 0)
        self.goal_id = 0

        self.offset = [0, 0]

        self.route = []

    def set_offset(self, offset):
        self.offset[0] = offset[0]
        self.offset[1] = offset[1]

    def ar_goal_callback(self, msg):
        """
        1. Check that it's the AR tag we are looking for
        2. Filter out dodgy values
            - are values within an absolute range?
            - standard deviation? idk
        3. Transform pose of tag relative to rover into global pose of tag
        
        """
        if msg.id != self.goal_id: return

        pose = msg.pose.pose.position
        
        local_pose = np.array([pose.x, pose.y])

        # tracking cam extrinsics are included in global pose as 0, 0 is the centre of the rover
        extrinsics = np.array(tracking_camera_extrinsics)[:2]
        local_pose -= extrinsics

        # distance from centre of rover to AR tag
        distance = (np.dot(local_pose, local_pose)) ** 0.5

        if not (min_ar_distance <= distance <= max_ar_distance):
            return

        # translate step
        rot_mat = np.array([[np.cos(self.state.yaw), -np.sin(self.state.yaw)], [np.sin(self.state.yaw), np.cos(self.state.yaw)]])
        local_pose.reshape(2, 1)
        
        global_pose = np.matmul(rot_mat, local_pose).reshape(2) + np.array([self.state.x, self.state.y])

        # goal diff for logging
        goal_diff = ((self.goal[0] - global_pose[0]) ** 2 + (self.goal[1] - global_pose[1]) ** 2) ** 0.5

        iD = msg.id

        if goal_diff > 0.02:
            self.get_logger().info("found tag: x=" + str(global_pose[0]) + " | y=" + str(global_pose[1]))
            odom = Odometry()
            odom.pose.pose.position.x = global_pose[0]
            odom.pose.pose.position.y = global_pose[1]
            odom.header.frame_id = main_frame
            odom.header.stamp = self.get_clock().now().to_msg()
            self.alvar_publisher.publish(odom)
            self.goal = global_pose[0], global_pose[1]
            self.get_logger().info("Updated planning goal (AR tag): x=" + str(global_pose[0]) + "| y=" + str(global_pose[1]))


    def manual_goal_callback(self, msg):
        """
        1. Check that it's the AR tag we are looking
        2. Filter out dodgy values
            - are values within an absolute range?
            - standard deviation? idk
        3. Transform pose of tag relative to rover into global pose of tag
        
        
        """
        print("\n\n\n\n")
        position = msg.position
        iD = msg.id

        self.get_logger().info("Next goal x=" + str(position.x) + " | y=" + str(position.y))
        self.goal = (position.x, position.y)
        self.at_goal = False
        self.goal_id = iD

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

        if not self.at_goal and distance((self.state.x, self.state.y), self.goal) < goal_achieved_distance:
            # WOooo!
            self.achieved_goal()
        

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
                self.get_logger().error("FUCK")
                return
        if status == PathPlanner.A_STAR_SUCCESS: self.get_logger().info("A* found safe path")

    def achieved_goal(self):
        """
        Inform the operators that we think we have achieved a goal,
        and update the state of the planner accordingly.
        """
        self.at_goal = True
        self.waypt_publisher.publish(Waypoints())
        self.get_logger().info(f"GOAL ACHIEVED: ({self.goal[0]}, {self.goal[1]}) [id = {self.goal_id}]")

    def get_path(self, _map):
        """
        Repeatedly run A* on the updated rover pose and map to continually redetermine the optimal path.
        Called on a clock initialised in the add_destination method
        """
        if self.at_goal: 
            self.get_logger().info("No goal to navigate to")
            return

        self.start = (self.state.x - self.offset[0], self.state.y - self.offset[1])
        local_goal = (self.goal[0] - self.offset[0], self.goal[1] - self.offset[1]) 

        self.length = _map.shape[0]
        self.width = _map.shape[1]

        self.length_meters = int(_map.shape[0] * self.resolution)
        self.width_meters = int(_map.shape[1] * self.resolution)
        
        self.route = np.array(a_star(_map, self.get_grid_coord(self.start), self.get_grid_coord(local_goal), self.resolution, self.padding_dist_m))
        status = self.route[-1, 0]
        self.route = self.route[:-1]

        route_coordinates = np.array(self.get_local_coords_route(self.route))
        route_coordinates[:] += np.array(self.offset)
        waypoints = Waypoints()
        #route_coordinates = [route_coordinates[-1] if len(route_coordinates) <= 4 else route_coordinates[4::5]]

        #self.get_logger().warn(str(route_coordinates[0]))
        for wpt in route_coordinates[min(len(route_coordinates) -1, ignore_waypoints):]:
            # publishing waypoints in order 
            waypoint = Waypoint()
            waypoint.x = wpt[0]
            waypoint.y = wpt[1]
           
            if (not math.isnan(waypoint.x)) and (not math.isnan(waypoint.y)):
                waypoints.waypoints.append(waypoint)

        print(f"publishing list with {len(route_coordinates)} waypoints")
        self.waypt_publisher.publish(waypoints)
        
