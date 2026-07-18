#!/usr/bin/env python3

from python_control.limits.Limit import Limit
from logging import Logger

from python_control.controls.Direction import Direction
from python_control.controls.Control import Control

class OneAxisVelocityControl(Control):
    """Class to control a single axis motor"""

    def __init__(self, logger: Logger, direction: Direction = Direction.POSITIVE, velocity: float = 0.0, max_percent: float = 1.0, pos_limit: Limit = None, neg_limit: Limit = None):
        super().__init__(logger=logger)
        self.direction = direction # type: Direction
        self.velocity = velocity # type: float [0.0, 1.0]
        self.max_percent = max_percent # type: float [0.0, 1.0]
        self.pos_limit = pos_limit # type: Limit
        self.neg_limit = neg_limit # type: Limit

    def update_direction(self, direction: Direction):
        """Update the direction of the motor"""
        if (direction == Direction.POSITIVE or direction == Direction.NEGATIVE):
            self.direction = direction
        else:
            raise ValueError("Invalid direction")
        
    def update_velocity(self, velocity: float, ignore_limits: bool = False):
        """Updates the velocity of the motor, if ignore_limits is True, the limits are ignored"""
        if ignore_limits or not self.check_limits_hit():
            if (0.0 <= velocity <= 1.0):
                self.velocity = velocity
            else:
                self.velocity = 1.0 if velocity > 1.0 else 0.0  
        
    def update_max_percent(self, max_percent: float):
        """Update the max percent of the motor"""
        if (0.0 <= max_percent <= 1.0):
            self.max_percent = max_percent
        else:
            self.max_percent = 1.0 if max_percent > 1.0 else 0.0  

    def get_velocity(self):
        """Get the velocity of the motor"""
        return self.velocity
    

    def get_direction(self):
        """Get the direction of the motor"""
        return self.direction
    

    def get_max_percent(self):
        """Get the max percent of the motor"""
        return self.max_percent
        
    def stop(self):
        """Stop the motor"""
        self.velocity = 0.0

    def update_limit_pos(self, limit_pos: bool):
        """Update the positive limit of the motor"""
        self.limit_pos = limit_pos

    def update_limit_neg(self, limit_neg: bool):
        """Update the negative limit of the motor"""
        self.limit_neg = limit_neg

    def has_limits(self) -> bool:
        """Check if the motor has limits"""
        return self.limit_pos is not None and self.limit_neg is not None
    
    def check_limits_hit(self) -> bool:
        """
        Check if the limits of the motor have been hit
        Stops the motor if the limits have been hit
        :return: bool
        """

        if ((self.direction == Direction.POSITIVE and self.pos_limit and self.pos_limit.get_limit_hit()) or 
            (self.direction == Direction.NEGATIVE and self.neg_limit and self.neg_limit.get_limit_hit())):
            self.stop()
            return True
        else:
            return False
