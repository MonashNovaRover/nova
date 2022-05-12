__package__ = "autonomous"

from controller.controller_interface import ControllerInterface
from controller.drive_controller import DriveController
from controller.turning import TankTurner
from typing import Tuple
from math_utils.controller_math import *
from rclpy.node import Node
from core.msg import AutonomousGoal, Point2D
from config.ros_config import planning_destination_topic
import numpy as np
import math


class SearchController(ControllerInterface, Node):
    def __init__(self, is_gate: bool = False):
        super().__init__("Search Controller")
        self.is_gate = False
        self.search = False
        self.found_tags = False

        self.turn_on_spot = {
            "started": False,
            "completed": False,
            "start_yaw": 0
        }

        self.publisher = self.create_publisher(AutonomousGoal, planning_destination_topic, 0)
        self.turner = TankTurner()
        self.driver = DriveController()

        self.waypoints = interpolate_circle_points(8, 10)
        self.waypoint_counter = 0
        self.current_waypoint = None

    def get_drive_command(self, target_waypoint, state: State, goal: Tuple[float], gate: Tuple[Tuple[float]]):
        if not self.turn_on_spot["completed"]:
            drive = self.turn(state)
        else:
            # Aim for next point in the search circle
            if self.driver.completed() and not self.found_tags:
                self.publish_next_waypoint(goal)
                drive = {"drive": 0, "steer": 0}  # Do nothing for one cycle

            else:
                drive = self.driver.get_drive_command(target_waypoint, state, self.current_waypoint, gate)

        return drive

    def publish_next_waypoint(self, goal):
        waypoint = np.array(self.waypoints[self.waypoint_counter])

        waypoint += np.array(goal)
        self.current_waypoint = waypoint
        self.publish_waypoint(self.current_waypoint)
        self.waypoint_counter += 1

    def publish_waypoint(self, waypoint):
        msg = AutonomousGoal()
        position = Point2D
        position.x, position.y = waypoint[0], waypoint[1]
        msg.position = position
        self.publisher.publish(msg)

    def new_goal(self, goal):
        """
        Updates the goal of the SearchController as the gate has been found
        """
        self.driver.goal = goal
        self.current_waypoint = goal
        self.publish_next_waypoint(self.current_waypoint)
        self.found_tags = True

    def turn(self, state: State):
        """
        Spins in place for a full circle to scan immediately around the rover
        """
        start_yaw = self.turn_on_spot["start_yaw"]
        facing = np.array([np.cos(state.yaw), np.sin(state.yaw)])
        target = np.array([np.cos(start_yaw), np.sin(start_yaw)])

        if not self.turn_on_spot["started"]:
            self.turn_on_spot["started"] = True
            self.turn_on_spot["start_yaw"] = state.yaw
        elif spin_achieved(1, facing, target):
            self.turn_on_spot["completed"] = True

        steer_fraction, drive_fraction = self.turner.turn(math.pi / 2, facing, target)
        return {"drive": drive_fraction, "steer": steer_fraction}

    def completed(self):
        return not self.search or (self.found_tags and self.driver.completed())
