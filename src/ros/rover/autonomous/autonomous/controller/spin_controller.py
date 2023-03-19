__package__ = "autonomous"

from autonomous.math_utils.controller_math import *
import numpy as np
import math


class SpinController:
    def __init__(self, start_yaw, driver):
        self.start_yaw = start_yaw
        self.driver = driver
        self.completed = False

    def turn_in_place(self, current_steer, current_orientation, position_vector=None):
        """
        Spins in place for a full circle to scan immediately around the rover
        """
        target_orientation = np.array([np.cos(self.start_yaw), np.sin(self.start_yaw), 0])

        if spin_achieved(1, current_orientation, target_orientation):
            self.completed = True

        return self.driver.get_drive_command(np.pi, current_steer, position_vector, current_orientation)

    def is_completed(self):
        return self.completed
