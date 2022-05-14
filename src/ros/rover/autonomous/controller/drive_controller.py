__package__ = "autonomous"

from math_utils.controller_math import *
from config.runtime_params import *
from controller.controller_interface import ControllerInterface
from controller.turning import *
import numpy as np


class DriveController(ControllerInterface):
    def __init__(self):
        self.target_waypoint = None
        self.state = None
        self.turner = TankTurner()
        self.goal = None

    def get_drive_command(self, target_waypoint, state, goal=None, gate=None):

        self.state = state
        self.target_waypoint = target_waypoint
        self.goal = goal
        position_vector = np.array([self.state.x, self.state.y, 0])
        target_vector = np.array([self.target_waypoint[0], self.target_waypoint[1], 0])

        desired_orientation = target_vector - position_vector
        current_orientation = np.array([np.cos(self.state.yaw), np.sin(self.state.yaw), 0])

        yaw_diff = yaw_difference(current_orientation, desired_orientation)

        drive = self.get_drive(yaw_diff, position_vector, current_orientation)
        return drive

    def get_drive(self, yaw_diff, position_vector, current_orientation):
        steer_fraction = 0.0
        drive_fraction = 0.0
        if yaw_diff > min_yaw_difference:
            steer_fraction, drive_fraction = self.turner.turn(yaw_diff, position_vector, current_orientation)
        else:
            self.turner.reset()
            # drive in straight line toward waypoint at determined velocity
            drive_fraction = crow_fly_target_velocity((self.state.x, self.state.y), self.target_waypoint)

        return {'steer': steer_fraction, 'drive': drive_fraction}

    def completed(self) -> bool:
        if self.goal is None:
            return True
        dist_to_target = distance(self.goal, (self.state.x, self.state.y))
        result = dist_to_target < goal_achieved_distance

        if result:
            self.goal = None
        return result
