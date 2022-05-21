__package__ = "autonomous"
"""
Some day, we will refactor this... for now, good luck
"""
from abc import *
from math_utils.controller_math import *
from config.runtime_params import *
import numpy as np


class Turner(ABC):
    @abstractmethod
    def turn(self, yaw_difference, position_vector, current_orientation):
        pass

    def reset(self):
        pass


class YawStarTurner(Turner):
    def __init__(self):
        # yaw_star params
        self.MAX_YAW = np.pi / 7.5
        self.MAX_TRAVERSAL_DISTANCE = 0.3
        self.target_yaw = 0
        # Variables
        self.star_state = 0
        self.first_drive = True
        self.direction = -1

        self.sign = None
        self.target_pose = None

    def turn(self, yaw_difference, position_vector, current_orientation):
        steer_fraction, drive_fraction = 0, 0

        if abs(yaw_difference) > self.MAX_YAW:
            # Big turn, either drive straight or turn
            if self.star_state == 0:
                # From straight line to first yaw
                self.target_yaw = np.sign(yaw_difference) * (abs(yaw_difference) - self.MAX_YAW)
                steer_fraction = tank_turn_target_yaw_rate(yaw_difference)
                drive_fraction = turn_drive_fraction
                # Update state
                self.star_state = 1

            elif self.star_state == 1:
                # Check if keep yawing
                if abs(yaw_difference) > abs(self.target_yaw):
                    # Keep turning
                    steer_fraction = tank_turn_target_yaw_rate(yaw_difference)
                    drive_fraction = turn_drive_fraction
                else:
                    # Swap to drive mode
                    dist = 0.5 * self.MAX_TRAVERSAL_DISTANCE if self.first_drive else self.MAX_TRAVERSAL_DISTANCE
                    self.star_state = 2
                    self.target_yaw = 0
                    self.target_pose = np.array([position_vector[0] + dist * self.direction * current_orientation[0],
                                                 position_vector[1] + dist * self.direction * current_orientation[1],
                                                 0])

                    self.sign = np.sign(np.dot((self.target_pose - position_vector), current_orientation))

            elif self.star_state == 2:
                # Check if keep driving
                dist = distance(position_vector, self.target_pose)

                sign_new = np.sign(np.dot((self.target_pose - position_vector), current_orientation))
                if abs(dist) > 0.1 and self.sign == sign_new:
                    # Keep driving
                    drive_fraction = straight_drive_fraction * self.direction
                    steer_fraction = 0.0
                else:
                    # Return to turning
                    steer_fraction = 0.0
                    drive_fraction = 0.0

                    # Update variables
                    self.direction = self.direction * -1
                    self.star_state = 0
                    self.first_drive = False

        elif self.MAX_YAW > abs(yaw_difference) > min_yaw_difference:
            # Turn on the spot
            steer_fraction = tank_turn_target_yaw_rate(yaw_difference)
            drive_fraction = turn_drive_fraction

        return steer_fraction, drive_fraction

    def reset(self):
        # Reset constants
        self.star_state = 0
        self.first_drive = True
        self.direction = -1


class TankTurner(Turner):
    def turn(self, yaw_difference, position_vector=None, current_orientation=None):
        return tank_turn_target_yaw_rate(yaw_difference), 0

    def reset(self):
        # do nothing
        return
