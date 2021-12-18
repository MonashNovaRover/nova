#!/usr/bin/env python3
import numpy as np
import time
from queue import PriorityQueue

import rclpy
## NOTE should probably call these something else since they are not only used by controller
from utils.controller_params import *
from utils.controller_math import *

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
    def __init__(self, dest, map2d):
        super().__init__("path_planner_node")
        
        # way-point publisher publishes a bunch of waypoints at once (hence using the 2D map dataype
        self.waypt_publisher = self.create_publisher(Waypoints, "autonomous/goals", 10)
        
        self.pose_subscriber = self.create_subscription(RoverPose, "autonomous/pose", self.update_pose, 10)

        self.map2d = map2d
        
        self.map2d.grid

        # exhaustive list of class attributes
        self.x_length_meters = self.map2d.length
        self.y_length_meters = self.map2d.width

        # x and y dimensions of the map in pixels
        self.scale_x = self.map2d.grid.shape[0]
        self.scale_y = self.map2d.grid.shape[1]

        # in case we want to test path planning without a controller
        self.state = State()
        
        self.route = []
        
        # create empty state

        # re running A* every second to re-evaluate
        self.timer = self.create_timer(a_star_rate, self.get_path)

    def get_grid_coord(self, position):
        return int((position[0] + self.map2d.length / 2) / self.map2d.resolution), \
               int((position[1] + self.map2d.width / 2) / self.map2d.resolution)

    def get_float_position(self, coord):


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

    def aStar(self, start, goal, weight=5, version="octile"):
        start = (start[0] + self.scale_x//2, start[1] + self.scale_y//2) 
        goal = (goal[0] + self.scale_x//2, goal[1] + self.scale_y//2)    

        print(start)
        t = time.time()
        # ways in which a coordinate can be expanded
        if version == "manhattan":
            neighbors = [(0, 1), (0, -1), (1, 0), (-1, 0)]
        else:
            neighbors = [(0, 1), (0, -1), (1, 0), (-1, 0), (-1, -1), (-1, 1), (1, -1), (1, 1)]
        # nodes which have been expanded (and thus form part of an optimal set of paths)
        closed_set = []
        # basically a way to store back pointers so we can recursively determine what a path is
        came_from = {}
        # dictionary to store g_scores - also serves as a record of nodes in open list
        g_score = {start: 0}
        # stores all the explored nodes in priority order of least g_score + f_score (should use a min heap)
        min_queue = PriorityQueue()

        # add the starting node to the queue
        min_queue.put((self.heuristic(start, goal), start))

        # while the heap of items still has things to look through
        while not min_queue.empty():
            expand = min_queue.get()[1]
            closed_set.append(expand)
            # goal check - necessary to test at goal given lack of consistent heuristic
            if expand == goal:
                break

            # children
            candidate_children = [(expand[0] + n[0], expand[1] + n[1]) for n in neighbors]
            candidate_children = [n for n in candidate_children if 0 < n[0] < len(self.map2d.grid) and 0 < n[1] < len(
                    self.map2d.grid[0]) and n not in closed_set and not self.map2d.grid[n[0]][n[1]]]
            for node in candidate_children:

                # lazy evaluation prevents key error
                if (node in g_score.keys() and g_score[expand] + 1 < g_score[node]) or (node not in g_score.keys()):
                    # why am I not using sqrt(2) distance for diagonal paths?
                    if version == "manhattan":
                        g_score[node] = g_score[expand] + 1
                    else:
                        # if we have done a diagonal movement, the cost to add is sqrt 2 - else it is unit 1
                        diag = abs(node[0] - expand[0]) + abs(node[1] - expand[1])
                        g_score[node] = g_score[expand] + (2 ** 0.5 if diag == 2 else 1)
                    came_from[node] = expand
                    min_queue.put((g_score[node] + self.heuristic(node, goal) * weight, node))

        # construct path from back pointers
        path = []
        node = goal
        while not(node == start):
            path.append(node)
            node = came_from[node]
        path.append(start)
        path.reverse()
        print("path: " + str(path))
        print("A* took: " + str(time.time() - t))
        self.route = path
        return path

    def get_local_coords_route(self, route):
        """
        Turning a route in pixel coordinates into one in metric coordinates
         - Modified for new map - have to check it works
        """
        return [(x * (float(self.x_length_meters) / self.scale_x),
                 y * (float(self.y_length_meters) / self.scale_y)) for (x, y) in route]

    def stringPull(self, raw_points):
        t = time.time()
        if len(raw_points) < 2:
            return raw_points

        safety_radius = int(corner_padding * self.scale_x / self.x_length_meters)
        print("pulling strings in the background")
        # pruned_list = [raw_points[0]]
        pruned_list = [np.array(raw_points[0])]

        # start of candidate string pull
        i = 1
        while i < len(raw_points):
            # end of candidate string pull
            obstacle = None

            dist = self.heuristic(pruned_list[-1], raw_points[i], heuristic_type="euclidean")
            if dist > safety_radius + 0.5:
                # NOTE: danger! we only start looking for obstacles outside the "padding" radius
                # from the last obstacle. On spiky / noisy maps this may be a bad assumption!
                for c in range(safety_radius, int(np.ceil(dist) * 2)):
                    # checking every .5 units to reduce chance of missing obstacles
                    unit_x = 0.5 * (raw_points[i][0] - pruned_list[-1][0]) / dist
                    unit_y = 0.5 * (raw_points[i][1] - pruned_list[-1][1]) / dist

                    # np.round() before int() to get the nearest point, rather than rounding down
                    candidate_x = int(np.round(pruned_list[-1][0] + c * unit_x))
                    candidate_y = int(np.round(pruned_list[-1][1] + c * unit_y))
                    if self.map2d.grid[candidate_x][candidate_y]:
                        obstacle = np.array((candidate_x, candidate_y))
                        break

            if obstacle is not None:
                if time.time() - t > 5:
                    break
                # converting to numpy array for convenience later
                pruned_list.append(obstacle)
                # don't increment i until we check we can get to the old point from new waypoint
                continue 
            
            i += 1

        pruned_list.append(np.array(raw_points[-1]))
        print("String pulling took: " + str(time.time() - t))
        print(pruned_list)
        return pruned_list

    def pad_corners(self, turning_points):
        """
        Takes all the turning points generated by string pulling of an A* path and modifies them so the
        rover avoids all corners by a constant radius. Approximates the curved path around each padding cricle
        with three waypoints.
        """
        t = time.time()

        if len(turning_points) <= 2:
            print("No corners to pad")
            return turning_points

        vectors = [turning_points[i + 1] - turning_points[i] for i in range(len(turning_points) - 1)]
        angles = [vector_argument(vector) for vector in vectors]
        # important for seeing which side of the padding circle we need to drive to if we want to avoid the obstacle
        angle_changes = [yaw_difference(angle, angles[i + 1]) for i, angle in enumerate(angles[:-1])]
        
        # only including waypoints with a non-trivial angle change
        new_turning_points = [turning_points[0]]
        for i, d_ang in enumerate(angle_changes):
            if abs(d_ang) > corner_angle_threshold:
                new_turning_points.append(turning_points[i + 1])
        
        new_turning_points.append(turning_points[-1])

        if len(new_turning_points) < len(turning_points):
            return self.pad_corners(new_turning_points)

        padded_path = [turning_points[0]]

        # adjusting each of the turning points - all string-pulled points except the destination
        for i in range(len(vectors) - 1):
            point = turning_points[i + 1]

            vec_to_pt = vectors[i]
            vec_from_pt = vectors[i + 1]

            r = int(corner_padding * self.scale_x / self.x_length_meters)
            d1 = np.sqrt(np.dot(vec_to_pt, vec_to_pt))
            d2 = np.sqrt(np.dot(vec_from_pt, vec_from_pt))

            if i == 0:
                # the first point on a padding circle - angles are a bit different to when we are travelling
                # between two circles
                p1 = point + radial_vec_to_tangent(r, d1, angles[i], angle_changes[i])
            else:
                p1 = point + radial_vec_to_common_circle_tangent(r, d1, angles[i], angle_changes[i], angle_changes[i - 1])
    
            if i == len(turning_points) - 3:
                p2 = point + radial_vec_to_tangent(r, d2, angles[i + 1] - np.pi, -angle_changes[i])
            else:
                radial_vec = radial_vec_to_common_circle_tangent(r, d2, angles[i + 1] - np.pi, -angle_changes[i], -angle_changes[i + 1])
                p2 = point + radial_vec

            print("p1 = " + str(p1))
            print(p2)
            padded_path.append(p1)
            padded_path = interpolate_circle_circumference(r, p2, point, circle_interpolation_num_points, padded_path, angle_changes[i])
            padded_path.append(p2)

        padded_path.append(turning_points[-1])

        print("path with padding: " + str(padded_path))
        print("path padding took: " + str(time.time() - t) + " s")

        return np.array(padded_path)

    def clear_path_to_first_waypoint(self, padded_path, distance_frac, num_rays, recursion_depth=5):
        """
        Takes in a list of waypoints with padding around all the sharp corners. This preliminary list
        does not avoid any low-angle corners or corners that weren't detected by string pulling.
        This algorithm recursively checks that the path to the first waypoint in the list is clear. If
        an obstacle is encountered on either side of the rover, a new waypoint is inserted to avoid it.
        As path planning re-runs regularly, we only need to check to the first waypoint planned.

        :param: distance_frac: the fraction of the corner_padding distance to move the new waypoint off course by
        """

        if distance_frac < 0.125 or recursion_depth == 0 or np.isnan(padded_path[0][0]) or np.isnan(padded_path[1][0]):
            return padded_path

        start_point = padded_path[0]

        dist = distance(start_point, padded_path[1])
        # vector between start location and first waypoint
        parallel_vec = padded_path[1] - start_point
        
        parallel_angle = vector_argument(parallel_vec)
        r = int(corner_padding * 0.8 * self.scale_x / self.x_length_meters)
        # vector between the centre line of the rover and either one of its sides
        perp_vec = r * np.array([np.cos(parallel_angle + np.pi / 2), np.sin(parallel_angle + np.pi / 2)])

        found_obstacle = False
        side_step_vec = np.array([0., 0.])

        for c in range(r, int(np.ceil(dist))):
            unit_vec = parallel_vec / dist     # unit vector in direction of vector to waypoint
            new_waypt = (start_point + unit_vec * c)    # waypoint we will place if we need to avoid an obstacle
            
            for i in range(num_rays):
                candidate_1 = (np.round(start_point + perp_vec / (i + 1) + unit_vec * c)).astype(int)
                candidate_2 = (np.round(start_point - perp_vec / (i + 1) + unit_vec * c)).astype(int)

                # If the side of the rover would go off the map, or it hits an obstacle
                if candidate_1[0] not in range(self.scale_x) or candidate_1[1] not in range(self.scale_y) or self.map2d.grid[candidate_1[0]][candidate_1[1]]:
                    side_step_vec = -perp_vec * distance_frac * (num_rays - i) / num_rays
                    found_obstacle = True
                
                # check the same for the other side
                # NOTE: this algorithm assumes we will never find an obstacle on both sides
                elif candidate_2[0] not in range(self.scale_x) or candidate_2[1] not in range(self.scale_y) or self.map2d.grid[candidate_2[0]][candidate_2[1]]:
                    side_step_vec = perp_vec * distance_frac * (num_rays - i) / num_rays
                    found_obstacle = True

            if found_obstacle:
                break

        if found_obstacle:
            new_waypt = np.round(new_waypt + side_step_vec).astype(int)
            padded_path = np.insert(padded_path, 1, new_waypt, 0)
            return self.clear_path_to_first_waypoint(padded_path, distance_frac, num_rays, recursion_depth-1)

        else:
            return padded_path


    def get_path(self, weight=5):
        """
        Repeatedly run A* on the updated rover pose and map to continually redetermine the optimal path.
        Called on a clock initialised in the add_destination method
        """

        self.start = (self.state.x, self.state.y)
        self.scale()

        print("Running A*")
        print("start: " + str(self.pixel_start))
        print("goal: " + str(self.pixel_goal))

        self.route = self.aStar(self.pixel_start, self.pixel_goal, weight)

        self.route = self.stringPull(self.route)

        self.route = self.pad_corners(self.route)

        self.route = self.clear_path_to_first_waypoint(self.route, 1.0, 1)

        route_coordinates = self.get_local_coords_route(self.route)

        waypoints = Waypoints()

        for wpt in route_coordinates:
            # publishing waypoints in order 
            waypoint = Waypoint()
            waypoint.x = wpt[0]
            waypoint.y = wpt[1]

            waypoints.waypoints.append(waypoint)

        print("publishing waypoint!")
        self.waypt_publisher.publish(waypoints)

        print("route has " + str(len(route_coordinates)) + " points")

