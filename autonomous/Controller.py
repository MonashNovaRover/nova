#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This script is the controller node for the rover 
which receives the destination and processed map, 
then publishing the drive command for movement.
Receives pose updates and waypoints via subscribers
and publishes drive commands. Converted to Ros2 by
Max Tory from initial code by Aidan Pritchard and 
Liam Whittle
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: Controller
TOPICS:
  - /D435/depth/color/points [sensor_msgs.msg.PointCloud2]
SERVICES:
  - None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        autonomous
AUTHOR(S):      Max Tory
CREATION:       07/12/2021
EDITED:         07/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
from controller_math import *
from controller_params import *
from core.msg import DriveCmd, RoverPose, Waypoint
import sys

"""
TODO: update led according to distance?
TODO: test rate object
TODO: test all publishers and subscribers
TODO: investigate more efficient/accurate drive control methods than repeated tank turning and forward driving
"""

class Controller(Node):
    """
    Controls the movement of the rover between waypoints determined by the path planner.
    Receives updates about the current pose of the rover and the waypoints to 
    navigate between via ros topics ------. Publishes drive commands to ---- 
    """
    def __init__(self):
        super().__init__('autonomous_controller_node')

        self.state = State()     # from controller_math
        self.waypoints = []
        self.max_distance = 0.0001      # furthest distance to an object? not sure

        self.drive_cmd_publisher = self.create_publisher(DriveCmd, "auto_drive_commands", 10)
        self.pose_subscriber = self.create_subscription(RoverPose, "autonomous/pose", self.update_pose, 10)
        self.waypt_subscriber = self.create_subscription(Waypoint, "autonomous/goals", self.add_waypoint, 10)

        # Controls the rate at which drive commands are sent - sleeps for the necessary time to maintain the frequency given
        # I think this is a ros2 implementation of the ros1 Rate object - need to check it works
        self.loop_rate = self.create_rate(controller_ros_rate, self.get_clock())

    def update_pose(self, msg):
        """
        Callback function that updates the current pose of the rover from data in the auto_command_pose_updates topic
        """
        self.state.x = msg.data.x
        self.state.y = msg.data.y
        self.state.yaw = msg.data.yaw
        self.state.velocity = msg.data.velocity
        self.state.angular_velocity = msg.data.angular_velocity

        # testing pose subscriber
        print("new pose: x = %.2f, y = %.2f, yaw = %.2f, vel = %.2f, omega = %.2f" % self.state.x, self.state.y, self.state.yaw, self.state.velocity, self.state.angular_velocity)

    def add_waypoint(self, msg):
        """
        Callback that appends the x-y position of a waypoint to the back of the waypoints list
        """
        self.waypoints.append([msg.data.x, msg.data.y])

        print("new waypoint: x = %.2f, y = %.2f" % msg.data.x, msg.data.y)

    def __publish(self, drive_fraction, angular_fraction):
        """
        Publishes drive commands to the auto_drive_commands topic
        :param: drive_fraction: [-1:1]
        :param: angular_fraction: [-1:1]
        """

        # construct message to publish
        drive_cmd_msg = DriveCmd()

        drive_cmd_msg.speed = drive_fraction

        drive_cmd_msg.steer = angular_fraction

        # publish to public topic
        self.drive_cmd_publisher.publish(drive_cmd_msg)

    @staticmethod
    def print_update(action_msg, heading_to, yaw_diff, dist):
        pad = 10
        sys.stdout.write("\r" + "Action: " + action_msg.ljust(pad) + " | heading to: " + str(heading_to).ljust(pad)
                          + " | yaw diff: " + str(round(yaw_diff, 4)).ljust(pad) + " | distance: " + str(round(dist, 4)).ljust(pad))
        sys.stdout.flush()

    def clear_waypoints(self):
        """
        empties the waypoints list - prevents further coordinates from being travelled to and allows path planning to be reset
        """
        self.waypoints = []

    def go_to_waypoint(self, waypoint):
        """
        Drives to a given waypoint in a straight line. Begins by tank turning (turning in place with 0 drive fraction)
        until facing in the direction of the target waypoint. Then drives forwards until waypoint is reached. Continually
        re-calculates target yaw and yaw_diff with each iteration to ensure the rover is not going off course.
        """
        while distance((self.state.x, self.state.y), waypoint) >= min_waypoint_distance:
            # calculate target yaw and signed yaw difference using the controller_math module
            target_yaw = desired_heading((self.state.x, self.state.y), waypoint)
            yaw_diff = yaw_difference(self.state.yaw, target_yaw)

            completed_turn = False

            # re-adjusts yaw to ensure we aren't going off track
            while abs(yaw_diff) >= (min_yaw_difference / 2.0):
                # Steering the rover to face in the desired direction before driving forward
                target_yaw = desired_heading((self.state.x, self.state.y), waypoint)
                yaw_diff = yaw_difference(self.state.yaw, target_yaw)

                steer_fraction = tank_turn_target_yaw_rate(self.state.yaw, target_yaw)

                self.__publish(0, steer_fraction)

                Controller.print_update("yawing", waypoint, yaw_diff, distance((self.state.x, self.state.y), waypoint))

                completed_turn = True

                # waits until 0.1 seconds since last sleep to publish next command
                self.loop_rate.sleep()
                
            if completed_turn:
                # need to send a zero wheel command after turning before we drive
                self.__publish(0, 0)
                self.loop_rate.sleep()

            # drive in straight line toward waypoint
            drive_fraction = crow_fly_target_velocity((self.state.x, self.state.y), waypoint)
            self.__publish(drive_fraction, 0)
            self.loop_rate.sleep()

            Controller.print_update("heading", waypoint, yaw_diff, distance((self.state.x, self.state.y), waypoint))

        print("Reached way-point: " + str(waypoint))
        for _ in range(5):
            self.publish(0.0, 0.0)
            self.loop_rate.sleep()

    def control(self, ask_for_input=False):
        """
        Pops waypoints from the waypoints list and navigates to them consecutively by calling go_to_waypoint
        """
        while rclpy.ok():
            if self.waypoints:
                # get permission to go to next waypoint
                if ask_for_input and self.way_points:
                    input("Please press enter before heading to way-point: " + str(self.way_points[0]))

                self.go_to_waypoint(self.waypoints.pop(0))
            else:
                break

        self.__publish(0,0)


def main(args = None):
    rclpy.init(args=args)
    controller = Controller()

    while rclpy.ok():
        controller.control(True)
        rclpy.spin_once(controller)
    controller.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()