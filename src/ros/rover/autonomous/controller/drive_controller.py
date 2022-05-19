__package__ = "autonomous"

from math_utils.controller_math import *
from config.runtime_params import *
from controller.turning import *
import numpy as np


class DriveController:
    def __init__(self, turner):
        self.target_waypoint = None
        self.turner = turner

    def get_drive_command(self, yaw_diff, position_vector, current_orientation):
        steer_fraction = 0.0
        drive_fraction = 0.0
        if yaw_diff > min_yaw_difference:
            steer_fraction, drive_fraction = self.turner.turn(yaw_diff, position_vector, current_orientation)
        else:
            self.turner.reset()
            # drive in straight line toward waypoint at determined velocity
            drive_fraction = crow_fly_target_velocity((self.state.x, self.state.y), self.target_waypoint)

        return {'steer': steer_fraction, 'drive': drive_fraction}
