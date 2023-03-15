__package__ = "autonomous"

from autonomous.math_utils.controller_math import *
import numpy as np
import math


class SpinController:
    def __init__(self, start_yaw, turner):
        self.start_yaw = start_yaw
        self.turner = turner
        self.completed = False

    def turn_in_place(self, current_position, current_orientation):
        """
        Spins in place for a full circle to scan immediately around the rover
        """
        target_orientation = np.array([np.cos(self.start_yaw), np.sin(self.start_yaw), 0])

        if spin_achieved(1, current_orientation, target_orientation):
            self.completed = True

        steer_fraction, drive_fraction = self.turner.turn(math.pi / 2, current_position, current_orientation)
        return {"drive": drive_fraction, "steer": steer_fraction}

    def is_completed(self):
        return self.completed
