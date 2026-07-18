#!/usr/bin/env python3

from logging import Logger
from python_control.controls.Control import Control

class ContinuousOneAxisPositionControl(Control):
    """Class to control a single axis motor continuously"""

    def __init__(self, logger: Logger, max_angle: int, min_angle: int = 0):
        super().__init__(logger=logger)
        self.goal_position = None # type: int | None
        self.min_angle = min_angle  # type: int
        self.max_angle = max_angle  # type: int

    def get_goal_position(self):
        """Get the target position of the motor"""
        return self.goal_position

    def get_max_angle(self):
        """Get the max angle of the positional motor"""
        return self.max_angle

    def displace(self, displacement: int):
        """Apply displacement to the target position of the motor"""
        if self.goal_position is None:
            return
        self.set_position(self.goal_position + displacement)

    def set_position(self, new_pos: int):
        """Set the target position of the motor"""
        self.goal_position = max(self.min_angle, min(new_pos, self.max_angle))

    def stop(self):
        pass
