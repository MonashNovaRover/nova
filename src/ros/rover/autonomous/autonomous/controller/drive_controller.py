__package__ = "autonomous"

from autonomous.math_utils.controller_math import *
from autonomous.config.runtime_params import *
from autonomous.controller.turning import *


class DriveController:
    def get_drive_command(self, yaw_diff, current_steer):
        """
        :param yaw_diff: shortest direction difference between current and target yaw
        :param current_steer: current steer value (-1 to 1)
        :param position_vector: current rover position in local map frame
        :param current orientation: orientation unit vector of rover in local map frame
        """
        target_steer = tank_turn_target_yaw_rate(yaw_diff)
        drive = drive_speed_from_turning_error(target_steer, current_steer)
        return drive, target_steer
