from autonomous.math_utils.controller_math import *
from autonomous.config.runtime_params import *
from autonomous.controller.turning import YawStarTurner, TankTurner
from enum import IntEnum


class TurningMode(IntEnum):
    TANK = 0
    PIVOT = 1


class DriveController:
    def __init__(self, turning_mode=TurningMode.PIVOT):
        self.turning_mode = turning_mode
        if turning_mode == TurningMode.TANK:
            self.turner = YawStarTurner()
        else:
            self.turner = None

    def get_drive_command(self, yaw_diff, current_radius, position_vector=None, current_orientation=None):
        """
        :param yaw_diff: shortest direction difference between current and target yaw
        :param current_steer: current steer value (-1 to 1)
        :param position_vector: current rover position in local map frame
        :param current orientation: orientation unit vector of rover in local map frame
        """
        if self.turning_mode == TurningMode.PIVOT:
            radius = tank_turn_target_yaw_rate(yaw_diff)
            drive = drive_speed_from_turning_error(radius, current_radius)
            radius = abs(radius)
            direction = int(-np.sign(yaw_diff))
        else:
            raise ValueError("Invalid turning mode")
        return drive, radius, direction
