#!/usr/bin/env python3
from path_planner import PathPlanner
from map2d_container import Map2DContainer
import rclpy
from mapper import Mapper


def main(args):
    rclpy.init(args=args)

    print("Welcome to fun car drive!")
    print("Setting up objects...")

    print("Input waypoints manually?")
    print("[0]: No - use autonomous path planning")
    print("[1]: Yes")

    manual_input = int(input("Input your decision: "))

    if manual_input:
        # TODO: Setup to allow manual inputs as well as path planning
        pass

    else:
        dest = [0.0, 0.0]
        dest[0] = float(input("Enter destination x coordinate: "))
        dest[1] = float(input("Enter destination y coordinate: "))
        
        length = 8
        width = 8
        resolution = 0.015

        # in this janky night-before-mvp we will be creating a map2d object which is shared by planner and mapper.
        # Mapper updates it, planner just reads from it.
        map2d = Map2DContainer(length=length, width=width, resolution=resolution)
        planner = PathPlanner(dest, map2d)
        mapper = Mapper(map2d, length=length, width=width, resolution=resolution)

        # This allows us to spin both nodes from main.py - we are kind of misusing ros nodes here but oh well it works
        executor = rclpy.executors.MultiThreadedExecutor()
        
        executor.add_node(planner)
        executor.add_node(mapper)

        # Spin in a separate thread
        executor.spin() 

        # rejoining threads before we shutdown
        rclpy.shutdown()


if __name__ == "__main__":
    main(args=None)
