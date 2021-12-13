#!/usr/bin/env python3
import numpy as np
import time
from queue import PriorityQueue
import rclpy
from controller_params import *
from rclpy.node import Node
import ArrayGrid
from core.msg import Waypoint
from os.path import expanduser

"""
Author: Aidan Pritchard and Liam Whittle

Date last updated: 18/2/2021 by Liam Whittle

Purpose: To perform A* path planning and string pulling on 2-d grid maps. 

Subscribed topic/s: 

Published topic/s:

Services:

"""


class PathPlanner(Node):
    def __init__(self, controller, array_grid: ArrayGrid, dest):
        super().__init__("path_planner_node")

        self.waypt_publisher = self.create_publisher(Waypoint, "autonomous/goals", 10)

        # controller that controls autonomous driving and grid that stores the current map
        self.controller = controller
        self.array_grid = array_grid

        # exhaustive list of class attributes
        self.x_length_meters = self.array_grid.length
        self.y_length_meters = self.array_grid.width

        # x and y dimensions of the map in pixels
        self.scale_x = self.array_grid.map.shape[0]
        self.scale_y = self.array_grid.map.shape[1]

        # gets 2d array of obstacles
        self.extract_obstacle_map(10)

        self.start = (self.controller.state.x, self.controller.state.y)
        self.goal = (dest[0], dest[1])

        self.route = []

        # calculations
        self.scale(self.scale_x, self.scale_y)

        # rto_lisiunning A* every second to re-evaluate
        self.timer = self.create_timer(a_star_rate, self.get_path)

    def extract_obstacle_map(self, layers: int):
        """
        Extracts an obstacle map by adding the bottom layers of the map
        """
        
        self.map = self.array_grid.map[:,:,self.array_grid.map.shape[2] // 2,0]

        # going from the middle of the map (which for flat ground, will be the pose of the camera)
        for i in range(self.array_grid.map.shape[2] // 2, self.array_grid.map.shape[2] // 2 + layers):
             self.map += self.array_grid.map[:,:,i,0]
        
        self.map = (self.map > 0.0).tolist()

        print("Map: ")
        print(self.map)

    def scale(self, scale_x, scale_y):
        """
        Calculate start and goal with respect of pixels, given local coordinates and total pixel height and width
        (0, 0) coordinates are located in the centre of the map, for both metric and pixel coordinates
        """ 
        self.pixel_goal = (int(self.goal[0] * self.scale_x / self.x_length_meters), int(self.goal[1] * self.scale_y/self.y_length_meters))

        #(self.scale_x - int(float(self.goal[0]) * (float(scale_x) / self.x_length_meters)), int(float(self.goal[1]) * (float(scale_y) / self.y_length_meters)))

        self.pixel_start = (int(self.start[0] * self.scale_x / self.x_length_meters), int(self.start[1] * self.scale_y/self.y_length_meters))
        
        #(self.scale_x - int(float(self.start[0]) * (float(scale_x) / self.x_length_meters)), (int(float(self.start[1]) * (float(scale_y) / self.y_length_meters))))
        
        print("Navigating from array-map-grid coordinates (%d, %d) to (%d, %d)" % (self.pixel_start[0], self.pixel_start[1], self.pixel_goal[0], self.pixel_goal[1]))
        
        return scale_x, scale_y

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

    def aStar(self, array, start, goal, weight=5, version="octile"):
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
            print("hi")
            # goal check - necessary to test at goal given lack of consistent heuristic
            if expand == goal:
                break

            # children
            candidate_children = [(expand[0] + n[0], expand[1] + n[1]) for n in neighbors]
            candidate_children = [n for n in candidate_children if 0 < n[0] < len(array) and 0 < n[1] < len(
                    array[0]) and n not in closed_set and not array[n[0]][n[1]]]
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
        path.reverse()
        print("path: " + str(path))
        print("A* took: " + str(time.time() - t))
        self.route = path
        return path

    def get_local_coords_route(self):
        """
        Turning a route in pixel coordinates into one in metric coordinates
         - Modified for new map - have to check it works
        """
        return [(x * (float(self.x_length_meters) / self.scale_x),
                 y * (float(self.y_length_meters) / self.scale_y)) for (x, y) in self.route]

    def stringPull(self, raw_points):
        t = time.time()
        if len(raw_points) < 2:
            return raw_points

        print("pulling strings in the background")
        # pruned_list = [raw_points[0]]
        pruned_list = []

        # start of candidate string pull
        i = 0
        while i < len(raw_points):
            # end of candidate string pull
            obstacle = False
            j = i + 1
            while j < len(raw_points):
                # question: would a straight line between r1[i] and r1[j] be obstructed?
                dist = self.heuristic(raw_points[i], raw_points[j], heuristic_type="straight_line")
                for c in range(0, int(np.ceil(dist))):
                    unit_x = (raw_points[j][0] - raw_points[i][0]) / dist
                    unit_y = (raw_points[j][1] - raw_points[i][1]) / dist
                    candidate_x = int(raw_points[i][0] + c * unit_x)
                    candidate_y = int(raw_points[i][1] + c * unit_y)
                    if self.map[candidate_x][candidate_y]:
                        obstacle = True
                        break
                if obstacle:
                    break
                j += 1

            if obstacle:
                pruned_list.append(raw_points[j - 1])
                i = j - 1
            i += 1

        pruned_list.append(raw_points[-1])
        print("String pulling took: " + str(time.time() - t))
        return pruned_list

    def run(self, weight):
        print("Running A*")
        print("start: " + str(self.pixel_start))
        print("goal: " + str(self.pixel_goal))

        self.route = self.aStar(self.map, self.pixel_start, self.pixel_goal, weight)

        # self.route = self.stringPull(self.map.numArr3, self.route)
        return self.route

    def get_path(self, weight=5):
        """
        Repeatedly run A* on the updated rover pose and map to continually redetermine the optimal path.
        Called on a clock initialised in the add_destination method

        NOTE: Will the map object be updated elsewhere? Or do we need to update it here?
        """
        # updating present coords with most recent rover pose
        self.start = (self.controller.state.x, self.controller.state.y)

        # create path object and run A*
        self.run(weight)

        self.route = self.stringPull(self.route)

        route_coordinates = self.get_local_coords_route()

        # clear all coordinates from the previous path before adding the newly created one
        self.controller.clear_waypoints()

        for wpt in route_coordinates:
            # publishing waypoints in order 
            waypoint = Waypoint()
            waypoint.x = wpt[0]
            waypoint.y = wpt[1]

            print("publishing waypoint!")
            self.waypt_publisher.publish(waypoint)

        print("route has " + str(len(route_coordinates)) + " points")

