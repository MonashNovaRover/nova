__package__ = "autonomous"

from controller.ControllerInterface import ControllerInterface
from math_utils.controller_math import *
import numpy as np

class TankTurnController(ControllerInterface):
    def __init__(self, logger):
        self.target_waypoint = None
        self.state = None
        self.logger = logger

        self.previously_turned = False

    def get_drive_command(self, target_waypoint, state, goal=None, gate=None):
        self.state = state
        self.target_waypoint = target_waypoint
        position_vector = np.array([self.state.x, self.state.y, 0])
        target_vector = np.array([self.target_waypoint[0], self.target_waypoint[1], 0])

        desired_orientation = target_vector - position_vector
        current_orientation = np.array([np.cos(self.state.yaw), np.sin(self.state.yaw), 0])

        yaw_diff = yaw_difference(current_orientation, desired_orientation)

        drive = self.tank_turn(yaw_diff, position_vector)
        self.log(drive)
        return drive

    def tank_turn(self, yaw_diff, position_vector):
        if abs(yaw_diff) >= min_yaw_difference:
            self.logger.info('Normal: turning, yaw_diff = ' + str(yaw_diff))
            # turn at a rate determined by the tank_turn_target_yaw_rate function
            steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
            drive_fraction = turn_drive_fraction
            self.previously_turned = True

        elif self.previously_turned:
            self.logger.info('Normal: swap')
            # need to send a zero wheel command after turning before we drive
            steer_fraction = 0.0
            drive_fraction = 0.0
            self.previously_turned = False

        else:
            self.logger.info('Normal: driving')
            # drive in straight line toward waypoint at determined velocity
            drive_fraction = crow_fly_target_velocity((self.state.x, self.state.y), self.target_waypoint)
            steer_fraction = 0.0

        return {'steer': steer_fraction, 'drive': drive_fraction}