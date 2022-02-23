#!/usr/bin/env python3
__package__ = "autonomous"
from planning.path_planner import PathPlanner
from controller.controller import Controller
import rclpy
from mapping.mapper import Mapper
from mapping.height_mapper import HeightMapper
from mapping.plane_mapper import PlaneMapper

def main(args):
    rclpy.init(args=args)

    print("Welcome to fun car drive!")
    dest = [0.0, 0.0]
    dest[0] = float(input("Enter destination x coordinate: "))
    dest[1] = float(input("Enter destination y coordinate: "))
    
    length = 20
    width = 20
    resolution = 0.1

    # in this janky night-before-mvp we will be creating a map2d object which is shared by planner and mapper.
    # Mapper updates it, planner just reads from it.
    planner = PathPlanner(dest, resolution)
    mapper = PlaneMapper(length=length, width=width, resolution=resolution, planner=planner)
    controller = Controller()

    # This allows us to spin both nodes from main.py - we are kind of misusing ros nodes here but oh well it works
    executor = rclpy.executors.MultiThreadedExecutor()
    
    executor.add_node(planner)
    executor.add_node(mapper)
    executor.add_node(controller)

    # Spin in a separate thread
    executor.spin() 

    # rejoining threads before we shutdown
    rclpy.shutdown()


if __name__ == "__main__":
    main(args=None)
