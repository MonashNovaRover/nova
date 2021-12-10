#!/usr/bin/env python3
from PathPlanner import PathPlanner
from Controller import Controller
from ArrayGrid import ArrayGrid
import threading
import rclpy


def main(args):

    rclpy.init(args = args)

    print("Welcome to fun car drive!")

    print("Setting up objects...")

    # controller object to plan the path according to the output of the path planner
    controller = Controller()

    # stores the map of obstacles we navigate through - dimensions and resolution are preliminary values
    grid = ArrayGrid(20.0, 20.0, 5.0, 0.05)

    print("Input waypoints manually?")
    print("[0]: No - use autonomous path planning")
    print("[1]: Yes")

    manual_input = int(input("Input your decision: "))

    print("Current pose: (" + str(controller.state.x) + ", " + str(controller.state.y) + ")")

    if manual_input:
        # TODO: Setup to allow manual inputs as well as path planning
        pass

    else:
        dest = [0.0, 0.0]
        dest[0] = float(input("Enter destination x coordinate: "))
        dest[1] = float(input("Enter destination y coordinate: "))
        
        planner = PathPlanner(controller, grid, dest)

        # This allows us to spin both nodes from main.py - we are kind of misusing ros nodes here but oh well it works
        executor = rclpy.executors.MultiThreadedExecutor()
        
        executor.add_node(planner)
        executor.add_node(controller)

        # Spin in a separate thread
        executor.spin() 

        # rejoinging threads before we shutdown
        rclpy.shutdown()


if __name__ == "__main__":
    main(args = None)
     