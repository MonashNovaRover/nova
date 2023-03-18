__package__ = "autonomous"

from autonomous.math_utils.controller_math import *
from autonomous.config.runtime_params import *
from autonomous.controller.turning import *
from autonomous.controller.GRUC import TurningMode
from autonomous.controller.turning import YawStarTurner


class DriveController:
    def __init__(self, turning_mode=TurningMode.PIVOT):
        self.turning_mode = turning_mode
        if turning_mode == TurningMode.TANK:
            self.turner = YawStarTurner()
        else:
            self.turner = None

    def get_drive_command(self, yaw_diff, current_steer, position_vector=None, current_orientation=None):
        """
        :param yaw_diff: shortest direction difference between current and target yaw
        :param current_steer: current steer value (-1 to 1)
        :param position_vector: current rover position in local map frame
        :param current orientation: orientation unit vector of rover in local map frame
        """
        if self.turning_mode == TurningMode.PIVOT:
            target_steer = tank_turn_target_yaw_rate(yaw_diff)
            drive = drive_speed_from_turning_error(target_steer, current_steer)
        elif self.turning_mode == TurningMode.TANK:
            if position_vector is None or current_orientation is None or self.turner is None:
                raise ValueError("Tank turning mode requires position vector and current orientation")
            else:   
                if abs(yaw_diff) > min_yaw_difference:
                    target_steer, drive = self.turner.turn(yaw_diff, position_vector, current_orientation)
                else:
                    self.turner.reset()
                    # drive in straight line toward waypoint at determined velocity
                    drive = straight_drive_fraction
                    target_steer = 0

        else:
            raise ValueError("Invalid turning mode")
        return drive, target_steer
