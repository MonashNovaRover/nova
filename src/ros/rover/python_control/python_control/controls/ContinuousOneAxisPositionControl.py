#!/usr/bin/env python3

from logging import Logger
from python_control.controls.Control import Control

class ContinuousOneAxisPositionControl(Control):
    """Class to control a single axis motor continuously"""

    def __init__(self, logger: Logger, max_angle: int, min_angle: int = 0):
        super().__init__(logger=logger)
        self.target_position = None # type: int | None
        self.min_angle = min_angle  # type: int
        self.max_angle = max_angle  # type: int

    def get_target_position(self):
        """Get the target position of the motor"""
        return self.target_position

    def displace(self, displacement: int):
        """Apply displacement to the target position of the motor"""
        self.set_position(self.target_position + displacement)

    def set_position(self, new_pos: int):
        """Set the target position of the motor"""
        self.target_position = max(self.min_angle, min(new_pos, self.max_angle))

    def stop(self):
        pass
