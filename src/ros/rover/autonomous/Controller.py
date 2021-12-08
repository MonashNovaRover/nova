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
from time import sleep
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
        self.target_waypoint = None
        self.previously_turned = False
        self.max_distance = 0.0001      # furthest distance to an object? not sure

        self.drive_cmd_publisher = self.create_publisher(DriveCmd, "auto_drive_commands", 10)
        self.pose_subscriber = self.create_subscription(RoverPose, "autonomous/pose", self.update_pose, 10)
        self.waypt_subscriber = self.create_subscription(Waypoint, "autonomous/goals", self.add_waypoint, 10)

        # Controls the rate at which drive commands are sent - sleeps for the necessary time to maintain the rate given
        self.timer = self.create_timer(controller_ros_rate, self.control)

    def update_pose(self, msg):
        """
        Callback function that updates the current pose of the rover from data in the auto_command_pose_updates topic
        """
        self.state.x = msg.x
        self.state.y = msg.y
        self.state.yaw = msg.yaw
        self.state.velocity = msg.velocity
        self.state.angular_velocity = msg.angular_velocity

        # testing pose subscriber
        print("new pose: x = %.2f, y = %.2f, yaw = %.2f, vel = %.2f, omega = %.2f" % (self.state.x, self.state.y, self.state.yaw, self.state.velocity, self.state.angular_velocity))

    def add_waypoint(self, msg):
        """
        Callback that appends the x-y position of a waypoint to the back of the waypoints list
        """
        self.waypoints.append([msg.x, msg.y])

        print("new waypoint: x = %.2f, y = %.2f" % (msg.x, msg.y))

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
        self.target_waypoint = None

    def tank_turn(self, yaw_diff, target_yaw):
        """
        turns the rover to face towards the target waypoint
        :returns: True if the rover needed to turn, False if it was already on course
        """

        # re-adjusts yaw to ensure we aren't going off track
        if abs(yaw_diff) >= (min_yaw_difference / 2.0):
            steer_fraction = tank_turn_target_yaw_rate(self.state.yaw, target_yaw)

            self.__publish(0.0, steer_fraction)

            Controller.print_update("yawing", self.target_waypoint, yaw_diff, distance((self.state.x, self.state.y), self.target_waypoint))
            
            self.previously_turned = True

            return True

        return False

    def go_to_target(self):
        """
        Publishes a single drive commmand to navigate to the current target waypoint. 
        Called every tick by the control method. Turns in place to face towards the waypoint, 
        or drives towards it in a straight line. If the rover has just finished turning, a
        single zero drive command is sent before driving begins.
        """
        # calculate target yaw and signed yaw difference using the controller_math module
        target_yaw = desired_heading((self.state.x, self.state.y), self.target_waypoint)
        yaw_diff = yaw_difference(self.state.yaw, target_yaw)

        if self.tank_turn(yaw_diff, target_yaw): return # turns the rover if necessary
            
        elif self.previously_turned:
            # need to send a zero wheel command after turning before we drive
            self.__publish(0.0, 0.0)
            return

        else:
            # drive in straight line toward waypoint
            drive_fraction = crow_fly_target_velocity((self.state.x, self.state.y), self.target_waypoint)
            self.__publish(drive_fraction, 0.0)

            Controller.print_update("heading", self.target_waypoint, yaw_diff, distance((self.state.x, self.state.y), self.target_waypoint))


    def control(self):
        """
        Called once every tick by the node's timer. Identifies the next target waypoint
        and calls navigate_to_waypoint, and determines when the rover has arrived
        """
        if self.target_waypoint == None:
            # There is currently no target - take the first waypoint on the list
            if self.waypoints:
                self.target_waypoint = self.waypoints.pop(0)
            else:
                return

        if distance((self.state.x, self.state.y), self.target_waypoint) >= min_waypoint_distance:
            # we have not yet arrived at the waypoint
            self.go_to_target()
        
        else:
            # If distance to the waypoint is lower than the threshold distance, we have arrived
            print("Reached way-point: " + str(self.target_waypoint))
            self.target_waypoint = None
            
            for _ in range(5):
                # stop for 5 seconds at waypoint
                self.publish(0.0, 0.0)
                sleep(1)

    """def control(self, ask_for_input=False):
        
        Pops waypoints from the waypoints list and navigates to them consecutively by calling go_to_waypoint
        
        while rclpy.ok():
            self.__get_updates()

            if self.waypoints and self.target_waypoint != None:
                # get permission to go to next waypoint
                if ask_for_input:
                    input("Please press enter before heading to way-point: " + str(self.waypoints[0]))

                self.go_to_waypoint(self.waypoints.pop(0))

        self.__publish(0.0,0.0)"""


def main(args = None):
    rclpy.init(args=args)
    controller = Controller()

    rclpy.spin(controller)

    controller.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()