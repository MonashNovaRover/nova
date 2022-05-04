__package__ = "autonomous"

from controller.controller_interface import ControllerInterface
from controller.drive_controller import DriveController
from controller.turning import TankTurner
from typing import Tuple
from math_utils.controller_math import *
from rclpy.node import Node
from core.msg import AutonomousGoal, Point2D
from config.ros_config import auto_goals_topic
import numpy as np
import math


class SearchController(ControllerInterface, Node):
    def __init__(self, is_gate: bool = False):
        super().__init__("Search Controller")
        self.tags = []

        self.is_gate = False
        self.search = False

        self.turn_on_spot = {
            "started": False,
            "completed": False,
            "start_yaw": 0
        }

        self.publisher = self.create_publisher(AutonomousGoal, auto_goals_topic, 0)
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
            if self.driver.completed():
                self.publish_next_waypoint(goal)
                drive = {"drive": 0, "steer": 0}  # Do nothing for one cycle

            else:
                drive = self.driver.get_drive_command(target_waypoint, state, self.current_waypoint, gate)

        return drive

    def publish_next_waypoint(self, goal):
        waypoint = np.array(self.waypoints[self.waypoint_counter])

        waypoint += np.array(goal)
        self.current_waypoint = waypoint

        msg = AutonomousGoal()

        position = Point2D
        position.x, position.y = waypoint[0], waypoint[1]
        id = -1  # no id for these interim points

        msg.id = id
        msg.position = position

        self.publisher.publish(msg)
        self.waypoint_counter += 1

    def new_goal(self, msg):
        x = msg.pose.pose.position.x
        y = msg.pose.pose.position.y

        if len(self.tags) == 0:
            self.tags.append((x, y))
            self.current_waypoint = (x, y)

        elif len(self.tags) == 1:
            previous_tag = self.tags[0]
            if distance(np.array(previous_tag), np.array(x, y)) > 1:
                self.tags.append((x, y))
                self.current_waypoint = average_tuple(self.tags)

        self.driver.goal = self.current_waypoint

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

    def found_tags(self):
        if self.is_gate:
            result = len(self.tags) == 2
        else:
            result = len(self.tags) == 1
        return result

    def completed(self):
        return not self.search or (self.found_tags() and self.driver.completed())

    def get_gate(self):
        if self.is_gate and len(self.tags == 2):
            gate = (self.tags[0], self.tags[1])
            return gate
        else:
            return None
