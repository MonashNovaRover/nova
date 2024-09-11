#!/usr/bin/env python3

from logging import Logger

from python_control.controls.Direction import Direction

from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl

class TimedOneAxisVelocityControl(OneAxisVelocityControl):
    """Class to control a single axis motor"""

    def __init__(self, logger: Logger, direction: Direction = Direction.POSITIVE, velocity: float = 0.0, max_percent: float = 1.0, time: float = 0.0):
        super().__init__(logger=logger, direction=direction, velocity=velocity, max_percent=max_percent)
        self.time = time # type: float
  
    def get_time(self) -> float:
        """Get the time to run the motor"""
        return self.time
    
    def set_time(self, time: float):
        """Set the time to run the motor"""
        self.time = time

    def stop(self):
        """Stop the motor"""
        self.velocity = 0.0
        self.time = 0.0
   