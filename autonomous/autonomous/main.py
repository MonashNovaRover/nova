#!/usr/bin/env python3
__package__ = "autonomous"
from autonomous.planning.path_planner import PathPlanner
import rclpy
from autonomous.mapping.python_height_mapper import HeightMapper
from autonomous.mapping.python_plane_mapper import PlaneMapper
from autonomous.mapping.height_plane_mapper import HeightPlaneMapper

def main(args):
    rclpy.init(args=args)

    print("Welcome to fun car drive!")
    resolution = 0.1

    # in this janky night-before-mvp we will be creating a map2d object which is shared by planner and mapper.
    # Mapper updates it, planner just reads from it.
    planner = PathPlanner(resolution)
    mapper = HeightPlaneMapper(resolution=resolution, planner=planner, camera=True)

    # This allows us to spin both nodes from main.py - we are kind of misusing ros nodes here but oh well it works
    executor = rclpy.executors.MultiThreadedExecutor()

    executor.add_node(planner)
    executor.add_node(mapper)

    try:
        executor.spin()
    except Exception as e:
        print(e)

    rclpy.shutdown()


if __name__ == "__main__":
    main(args=None)
