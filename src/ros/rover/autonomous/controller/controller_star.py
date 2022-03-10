#!/usr/bin/env python3

__package__ = "autonomous"

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
        self.drive_cmd_publisher = self.create_publisher(DriveInput, auto_drive_command_topic, 10)
        self.pose_subscriber = self.create_subscription(RoverPose, rover_pose_topic, self.update_pose, 10)
        self.waypt_subscriber = self.create_subscription(Waypoints, auto_goals_topic, self.add_waypoints, 10)
SERVICES:
  - None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        autonomous
AUTHOR(S):      Max Tory
CREATION:       07/12/2021
EDITED:         07/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO: 
- update led according to distance?
- test all publishers and subscribers
- investigate more efficient/accurate drive control methods than repeated tank turning and forward driving
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
from math_utils.controller_math import *
from config.runtime_params import *
from core.msg import DriveInput, RoverPose, Waypoints
import sys
import vis.path_vis as path_vis

from config.ros_config import rover_pose_topic
from config.ros_config import auto_drive_command_topic
from config.ros_config import auto_goals_topic


class Controller(Node):
    """
    Controls the movement of the rover between waypoints determined by the path planner.
    Receives updates about the current pose of the rover and the waypoints to 
    navigate between via ros topics autonomous/pose and autonomous/goals. Publishes drive 
    commands to auto_drive_commands
    """

    def __init__(self):
        super().__init__('autonomous_controller_node')

        self.state = State()  # from controller_math
        self.waypoints = []
        self.target_waypoint = None
        self.previously_turned = False
        self.max_distance = 0.0001  # furthest distance to an object? not sure
        
        # Star parameters
        # Statics
        self.MAX_YAW = np.pi / 7.5
        self.MAX_TRAVERSAL_DISTANCE = 1
        # Variables
        self.star_state = 0 
        self.first_drive = True
        self.direction = -1
        
        self.path_cloud = path_vis.PathCloud()

        self.drive_cmd_publisher = self.create_publisher(DriveInput, auto_drive_command_topic, 10)
        self.pose_subscriber = self.create_subscription(RoverPose, rover_pose_topic, self.update_pose, 10)
        self.waypt_subscriber = self.create_subscription(Waypoints, auto_goals_topic, self.add_waypoints, 10)

        # Controls the rate at which drive commands are sent - sleeps for the necessary time to maintain the rate given
        self.timer = self.create_timer(0.1, self.control)

    def update_pose(self, msg):
        """
        Callback function that updates the current pose of the rover from data in the auto_command_pose_updates topic
        """
        self.state.x = msg.x
        self.state.y = msg.y
        self.state.yaw = msg.yaw
        self.state.velocity = msg.velocity
        self.state.angular_velocity = msg.angular_velocity

    def add_waypoints(self, msg):
        """
        Callback that appends the x-y position of a waypoint to the waypoints list
        """
        self.waypoints = [[point.x, point.y] for point in msg.waypoints]

    def __publish(self, drive_fraction, angular_fraction):
        """
        Publishes drive commands to the auto_drive_commands topic
        :param: drive_fraction: [-1:1]
        :param: angular_fraction: [-1:1]
        """

        # construct message to publish
        drive_cmd_msg = DriveInput()

        drive_cmd_msg.speed = drive_fraction

        drive_cmd_msg.steer = angular_fraction

        # publish to public topic
        self.drive_cmd_publisher.publish(drive_cmd_msg)

    @staticmethod
    def print_update(action_msg, heading_to, yaw_diff, dist):
        pad = 10
        sys.stdout.write("\r" + "Action: " + action_msg.ljust(pad) + " | heading to: " + str(heading_to).ljust(pad)
                         + " | yaw diff: " + str(round(yaw_diff, 4)).ljust(pad) + " | distance: " + str(
            round(dist, 4)).ljust(pad))
        sys.stdout.flush()
    
    def go_to_target(self):
        """
        Publishes a single drive command to navigate to the current target waypoint.
        Called every tick by the control method. Turns in place to face towards the waypoint, 
        or drives towards it in a straight line. If the rover has just finished turning, a
        single zero drive command is sent before driving begins.
        """
        # calculate target yaw and signed yaw difference using the controller_math module
        position_vector = np.array([self.state.x, self.state.y, 0])
        target_vector = np.array([self.target_waypoint[0], self.target_waypoint[1], 0])
        
        desired_orientation = target_vector - position_vector
        current_orientation = np.array([np.cos(self.state.yaw), np.sin(self.state.yaw), 0])
        
        yaw_diff = yaw_difference(current_orientation, desired_orientation)
            
        #implement yaw_star
        if abs(yaw_diff) > MAX_YAW:
            #Big turn, either drive straight or turn
            if self.star_state == 0:
                #From straight line to first yaw
                self.target_yaw = np.sign(yaw_diff) * (abs(yaw_diff) - self.MAX_YAW)
                steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
                self.__publish(0.05, steer_fraction)
                Controller.print_update("Start star yawing", self.target_waypoint, yaw_diff,
                                distance((self.state.x, self.state.y), self.target_waypoint))
                #Update state
                self.star_state = 1
            elif self.star_state == 1:
                #Check if keep yawing
                if yaw_diff > self.target_yaw:
                    #Keep turning
                    steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
                    self.__publish(0.05, steer_fraction)
                    Controller.print_update("Keep star yawing", self.target_waypoint, yaw_diff,
                                distance((self.state.x, self.state.y), self.target_waypoint))
                else:
                    #Swap to drive mode
                    dist = 0.5 * self.MAX_TRAVERSAL_DISTANCE if self.first_drive else self.MAX_TRAVERSAL_DISTANCE
                    self.star_state = 2
                    self.target_yaw = 0
                    self.target_pose = [position_vector[0] + dist * self.direction * np.cos(current_orientation), 
                                       position_vector[1] + dist * self.direction * np.sin(current_orientation), 0]
                    self.__publish(0.0, 0.0)
                    Controller.print_update("Start star heading", self.target_waypoint, yaw_diff,
                                distance((self.state.x, self.state.y), self.target_waypoint))
            elif self.star_state == 2:
                #Check if keep driving
                dist = distance(position_vector, self.target_pose)
                if abs(dist) > 0.01:
                    #Keep driving
                    drive_fraction = 0.1 * direction
                    self.__publish(drive_fraction, 0.0)
                    Controller.print_update("star heading", self.target_waypoint, yaw_diff,
                                distance((self.state.x, self.state.y), self.target_waypoint))

                else:
                    #Return to turning
                    self.__publish(0.0, 0.0)
                    Controller.print_update("Finish star heading", self.target_waypoint, yaw_diff,
                                distance((self.state.x, self.state.y), self.target_waypoint))
                    
                    #Update variables
                    self.direction = direction * -1
                    self.star_state = 0
                    self.first_drive = False
        elif abs(yaw_diff) < self.MAX_YAW and abs(yaw_diff) > min_yaw_difference:  
            #Turn on the spot
            steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
            self.__publish(0.05, steer_fraction)
            Controller.print_update("yawing", self.target_waypoint, yaw_diff,
                        distance((self.state.x, self.state.y), self.target_waypoint))
        else:
            print('DONE')
            #Reset constants
            self.star_state = 0 
            self.first_drive = True
            direction = -1
 
            # drive in straight line toward waypoint at determined velocity
            drive_fraction = crow_fly_target_velocity((self.state.x, self.state.y), self.target_waypoint)
            self.__publish(drive_fraction, 0.0)

            Controller.print_update("heading", self.target_waypoint, yaw_diff,
                                    distance((self.state.x, self.state.y), self.target_waypoint))

    def control(self):
        """
        Called once every tick by the node's timer. Identifies the next target waypoint
        and calls navigate_to_waypoint, and determines when the rover has arrived
        """
        if not self.target_waypoint:
            # There is currently no target - take the first waypoint on the list
            if self.waypoints:
                self.target_waypoint = self.waypoints.pop(0)
            else:
                return

        if distance((self.state.x, self.state.y), self.target_waypoint) >= min_waypoint_distance:
            # we have not yet arrived at the waypoint
            self.go_to_target()

            # showing where we are aiming to drive to
            self.path_cloud.publish_path(self.waypoints)

        else:
            # If distance to the waypoint is lower than the threshold distance, we have arrived
            print("Reached way-point: " + str(self.target_waypoint))
            self.target_waypoint = None
            self.control()


def main(args=None):
    rclpy.init(args=args)
    controller = Controller()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


def controller_test():
    rclpy.init(args=None)
    controller = Controller()
    controller.waypoints = [[0, 0], [.7, .0], [.7, .7]]
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
    # controller_test()