from ArrayGrid import ArrayGrid
from PathPlanner import PathPlanner
import numpy as np
from time import time

from controller_params import *
import matplotlib.pyplot as plt
import rclpy

def generate_obstacles(this_map):
    for i in range(35):
        this_map[33][20 + i] = 1

    for i in range(20):
        this_map[48][50 + i] = 1

    for i in range(10):
        this_map[55 + i][28] = 1
    
    for i in range(20):
        this_map[15 + i][23] = 1

    return this_map

if __name__ == "__main__":
    rclpy.init(args = None)

    t1 = time()
    grid = ArrayGrid(3.5, 3.5, 1.0, 0.05)
    t2 = time()
    print ("grid took " + str(t2 - t1) + " s to make")

    this_map = [[0 for _ in range(70)] for _ in range(70)]
    
    this_map = generate_obstacles(this_map)

    for startx in [-1.5, -1., -0.5, 0., 0.5, 1.0, 1.5]:
        for starty in [-1.5, -1, -0.5, 0, 0.5, 1.0, 1.5]:
            this_map[int(startx * 20 + 35)][int(starty * 20 + 35)] = 2
            
            t3 = time()
            print("map obstacles took " + str(t3 - t2) + " s to make")

            planner = PathPlanner(None, grid, [1.45, 1.])

            t4 = time()
            print("planner took " + str(t4 - t3) + " s to make")

            planner.map = this_map

            planner.start = (startx, starty)
            planner.scale()

            path = planner.aStar(planner.pixel_start, planner.pixel_goal)
            path = planner.stringPull(path)
            padded_path = planner.pad_corners(path)

            route_coordinates = planner.get_local_coords_route(path)

            path = np.array(path)
            padded_path = np.array(padded_path)

            plt.figure()
            
            try:
                plt.plot(path[:,1], path[:,0], 'o')
                plt.plot(padded_path[:,1], padded_path[:,0])
            except Exception as e:
                print(e)
                print("path = " + str(path))
                print("padded_path = " + str(padded_path))
                continue
            
            plt.xlim = [-35, 35]
            plt.ylim = [-35, 35]

            plt.imshow(this_map)

            plt.savefig("x=%.1f_y=%.1f.png" % (startx, starty))

            this_map[int(startx * 20 + 35)][int(starty * 20 + 35)] = 0
    rclpy.shutdown()

