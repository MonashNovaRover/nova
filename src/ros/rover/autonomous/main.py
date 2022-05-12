#!/usr/bin/env python3
__package__ = "autonomous"
from planning.path_planner import PathPlanner
from controller.old_controller import Controller
import rclpy
from mapping.mapper import Mapper
from mapping.height_mapper import HeightMapper
from mapping.plane_mapper import PlaneMapper
from mapping.height_plane_mapper import HeightPlaneMapper
import time
import threading

def main(args):
    rclpy.init(args=args)

    print("Welcome to fun car drive!")
    length = 20 
    width = 20
    resolution = 0.1

    # in this janky night-before-mvp we will be creating a map2d object which is shared by planner and mapper.
    # Mapper updates it, planner just reads from it.
    planner = PathPlanner(resolution)
    mapper = HeightPlaneMapper(length=length, width=width, resolution=resolution, planner=planner, camera=True)
    controller = Controller()

    # This allows us to spin both nodes from main.py - we are kind of misusing ros nodes here but oh well it works
    executor = rclpy.executors.MultiThreadedExecutor()
    
    executor.add_node(planner)
    executor.add_node(mapper)
    executor.add_node(controller)

    executor.spin()

    rclpy.shutdown()


if __name__ == "__main__":
    main(args=None)
