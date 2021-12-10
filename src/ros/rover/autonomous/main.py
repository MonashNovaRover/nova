import PathPlanner, Controller, ArrayGrid
import rclpy
from rclpy.node import node
from core.msg import DriveCmd, RoverPose, way_points


if __name__ == "__main__":

    print("Welcome to fun car drive!")

    print("Setting up objects...")

    # controller object to plan the path according to the output of the path planner
    controller = Controller()

    # stores the map of obstacles we navigate through
    grid = ArrayGrid()

    print("Input waypoints manually?")
    print("[0]: No - use autonomous path planning")
    print("[1]: Yes")
    
    manual_input = int(input("Input your decision: "))

    if manual_input:
        print("Current pose: (" + str(controller.state.x) + ", " + str(controller.state.y) + ")")
        

        print("Calculating path...")
        path = PathPlanner.get_path((controller.state.x, controller.state.y), dest, weight=5)
        print("Path is: " + str(path))

        print("Adding way-points...")
        for point in path:
            controller.way_points.append(point)

    else:
        planner = PathPlanner(controller, grid)

        dest = input("Enter Destination as tuple: ")
        
        planner.add



    