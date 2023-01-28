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

    def get_turn_speed(self, yaw_diff):
        return big_turn_drive_fraction if abs(yaw_diff) > small_turn_angle else\
                small_turn_drive_fraction


class YawStarTurner(Turner):
    START_TURN = 0
    TURNING = 1
    DRIVING = 2
    def __init__(self):
        # yaw_star params
        self.MAX_YAW = np.pi / 7.5
        self.MAX_TRAVERSAL_DISTANCE = 0.3
        self.target_yaw = 0
        # Variables
        self.star_state = YawStarTurner.START_TURN
        self.first_drive = True
        self.direction = -1

        self.sign = None
        self.target_pose = None

    def turn(self, yaw_difference, position_vector, current_orientation):
        steer_fraction, drive_fraction = 0.0, 0.0
        current_yaw = np.arctan2(current_orientation[1], current_orientation[0])
        print(f"current yaw = {current_yaw}")

        if abs(yaw_difference) > self.MAX_YAW:
            # Big turn, either drive straight or turn
            if self.star_state == YawStarTurner.START_TURN:
                # From straight line to first yaw
                self.target_yaw = current_yaw + (np.sign(yaw_difference) * self.MAX_YAW)
                self.target_yaw %= 2 * np.pi
                self.target_yaw -= 2 * np.pi if self.target_yaw > np.pi else 0
                print(f"switching to target yaw of {self.target_yaw}")
                steer_fraction = tank_turn_target_yaw_rate(yaw_difference)
                drive_fraction = self.get_turn_speed(yaw_difference)
                # Update state
                self.star_state = YawStarTurner.TURNING

            elif self.star_state == YawStarTurner.TURNING:
                # pos if we are beyond the target yaw
                diff = (current_yaw - self.target_yaw) % (2 * np.pi)
                diff -= 2 * np.pi if diff > np.pi else 0
                abs_diff = np.sign(yaw_difference) * diff
                print(f"absolute difference to target yaw is {abs_diff}")
                if abs_diff < 0:
                    # Keep turning
                    steer_fraction = tank_turn_target_yaw_rate(yaw_difference)
                    drive_fraction = self.get_turn_speed(yaw_difference)
                else:
                    # Swap to drive mode
                    dist = 0.5 * self.MAX_TRAVERSAL_DISTANCE if self.first_drive else self.MAX_TRAVERSAL_DISTANCE
                    self.star_state = YawStarTurner.DRIVING
                    self.target_yaw = 0
                    self.target_pose = np.array([position_vector[0] + dist * self.direction * current_orientation[0],
                                                 position_vector[1] + dist * self.direction * current_orientation[1],
                                                 0])

                    self.sign = np.sign(np.dot((self.target_pose - position_vector), current_orientation))

            elif self.star_state == YawStarTurner.DRIVING:
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
                    self.star_state = YawStarTurner.START_TURN
                    self.first_drive = False
                print(f"yaw_star: driving")

        elif self.MAX_YAW > abs(yaw_difference) > min_yaw_difference:
            # Turn on the spot
            steer_fraction = tank_turn_target_yaw_rate(yaw_difference)
            drive_fraction = self.get_turn_speed(yaw_difference)
         

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
