#!/usr/bin/env
import numpy as np
import matplotlib.pyplot as plt
import time
from Queue import PriorityQueue
from ArrayMap import ArrayGrid
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


class PathPlanner:
    def __init__(self, controller, array_grid: ArrayGrid, end, x_length=20, y_length=20):
        """
        x is height, y is width
        """
        # NOTE - Max - why do we need to reverse x and y coords?
        # NOTE: we are REVERSING the coordinates of start to end so that path planning works

        self.controller = controller
        self.array_grid = array_grid

        # exhaustive list of class attributes
        self.x_length_meters = self.array_grid.length
        self.y_length_meters = self.array_grid.width

        # x and y coordinates of the map in pixels
        self.scale_x = self.array_grid.map.shape[0]
        self.scale_y = self.array_grid.map.shape[0]

        self.extract_obstacle_map(10)

        self.route = []

        # calculations
        self.scale(self.scale_x, self.scale_y)

    def add_destination(self, dest):
        """
        Adds destination node to plan path to (from present rover pose)
        """
        self.end = dest

    def extract_obstacle_map(self, layers: int):
        """
        Extracts an obstacle map by adding the bottom layers of the map
        """

        self.map = self.array_grid.map[:,:,0,0]
        for i in range(1: layers):
            self.map += self.array_grid.map[:,:,i,0]

    def scale(self, scale_x, scale_y):
        """
        Calculate start and goal with respect of pixels, given local coordinates and total pixel height and width
        """ 
        self.pixel_goal = (self.scale_x - int(float(self.end[0]) * (float(scale_x) / self.x_length_meters)),
                           int(float(self.end[1]) * (float(scale_y) / self.y_length_meters)))

        self.pixel_start = (self.scale_x - int(float(self.start[0]) * (float(scale_x) / self.x_length_meters)),
                           (int(float(self.start[1]) * (float(scale_y) / self.y_length_meters))))
        
        print("Navigating from pixel coordinates (%d, %d) to (%d, %d)" % (self.pixel_start, self.pixel_goal))
        
        return scale_x, scale_y

    """
    The manhattan heuristic works for movements up, down, left, right, and is an admissible and consistent heuristic
    The straight line heuristic works for any problem domain in 2-d euclidean space and is admissible and consistent
    The octile heuristic is admissible and consistent for octile movements where diagonal movements incur cost sqrt(2)
    """
    @staticmethod
    def heuristic(a, b, heuristic_type="distance"):
        if heuristic_type == "manhattan":
            return abs(a[0] - b[0]) + abs(a[1] - b[1])  # manhattan
        elif heuristic_type == "octile":
            return min(abs(a[0] - b[0]), abs(a[1] - b[1])) * (2 ** 0.5) + abs(abs(a[0] - b[0]) - abs(a[1] - b[1]))  # octile
        return np.sqrt((b[0] - a[0]) ** 2 + (b[1] - a[1]) ** 2)  # straight

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

            print(expand)

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
            print("path: " + str(path))
            node = came_from[node]
        path.reverse()
        print("A* took: " + str(time.time() - t))
        self.route = path
        return path

    def get_local_coords_route(self):
        return [((self.scale_x - x) * (float(self.x_length_meters) / self.scale_x),
                 y * (float(self.y_length_meters) / self.scale_y)) for (x, y) in self.route]

    def stringPull(self, num_map, raw_points):
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
                    if num_map.padded_obs_np[candidate_x][candidate_y]:
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

        self.route = self.aStar(self.map.padded_obs_np, self.pixel_start, self.pixel_goal, weight)

#         self.route = self.stringPull(self.map.numArr3, self.route)
        return self.route


def convert_grid_to_coord_list(np_arr, factor_x, factor_y):
    f = open("arc_crop_coords.csv", "a")
    for x in range(0, np_arr.shape[0]):
        for y in range(0, np_arr.shape[1]):
            if np_arr[x][y]:
                print("factor_x: " + str(factor_x) + " | val: " + str(x))
                f.write(str(float(x) * factor_x) + "," + str(float(y) * factor_y) + "\n")
    f.close()


def get_path(start, dest, weight=5):
    # create map object
    home_dir = expanduser("~")
    arc = ArrayMap(home_dir + '/catkin_ws/src/arc_auto/scripts/path_planning/arc_crop_bw_small.png')

    # create path object and run A*
    _path = PathPlanner(arc, start, dest)
    _path.run(weight)

    _path.route = _path.stringPull(arc, _path.route)

    if not _path.route:
        return []

    convert_grid_to_coord_list(arc.padded_obs_np, 20.0 / float(arc.dim_x), 20.0 / float(arc.dim_y))
    print(arc.padded_obs_np.shape[0])
    print(arc.padded_obs_np.shape[1])

    print("route: " + str(_path.get_local_coords_route()))
    _path.plot(arc.obs_np, route=_path.route)

    # get_local_coords_route returns a flipped version of coordinates
    return [(y, x) for (x, y) in _path.get_local_coords_route()]


if __name__ == "__main__":
    get_path((0, 0), (6.0, 6.0))
