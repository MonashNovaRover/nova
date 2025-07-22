#!/usr/bin/env python3

from logging import Logger
from python_control.controls.Control import Control
from python_control.sensors import IntegerSensor
from python_control.sensors.CommandSensor import CommandSensor

class OneAxisPositionControl(Control):
    """Class to control a single axis motor"""
    ZERO = "zero"

    def __init__(self, logger: Logger, positions: dict[str, int] = {}, position_sensor: IntegerSensor = None, zero_sensor: CommandSensor = None, max_angle: int = 0, offset: int = 0):
        super().__init__(logger=logger)
        positions[self.ZERO] = 0
        self.position_name = self.ZERO # type: str
        self.positions = positions # type: dict[str, int]
        self.position_sensor = position_sensor # type: IntegerSensor
        self.zero_sensor = zero_sensor # type: CommandSensor
        self.max_angle = max_angle # type: int
        self.offset = offset # type: int

    def get_max_angle(self):
        """Get the max angle of the motor if applicable"""
        return self.max_angle

    def get_offset(self):
        """Get the offset"""
        return self.offset

    def set_offset(self, new_val):
        """Set the offset"""
        self.offset = new_val

    def get_current_position(self):
        """Get the current position of the motor"""
        return self.position_sensor.get_sensor_value()

    def get_goal_position(self):
        """Get the position to move the motor to. Limited between 0 and self.max_angle."""
        return max(0, min(self.max_angle, self.positions[self.position_name] + self.offset))
    
    def get_position_name(self):
        """Get the name of the current position to move the motor to"""
        return self.position_name
    
    def get_positions(self):
        """Get the set of available positions of the motor"""
        return self.positions

    def add_position(self, name: str, position: int):
        """Add a position to the set of available positions"""
        self.positions[name] = position
    
    def remove_position(self, name: str):
        """Remove a position from the set of available positions"""
        del self.positions[name]

    def update_position(self, name: str):
        """Go to a specific position"""
        self.position_name = name

    def zero(self):
        """Zero the motor"""
        self.position_name = self.ZERO
    
    def valid_position(self, name: str):
        """Check if the motor is at the goal position"""
        return name in self.positions.keys()

    def distance_to_position(self) -> int:
        """Get the distance to the set position"""
        # TODO: Apply limits and offset
        return abs(self.positions[self.position_name] - self.position_sensor.get_sensor_value())
    
    def is_zeroed(self) -> bool:
        """Check if the motor is at the zero position"""
        return self.zero_sensor.get_sensor_value()

    def is_at_position(self) -> bool:
        """Check if the motor is at the specific position"""
        return self.position_sensor.get_sensor_value() == self.positions[self.position_name]
    
    def sensor_callbacks(self, frame):
        """Update the sensor values based on the frame"""
        if self.position_sensor is not None:
            self.position_sensor.frame_callback(frame)
        if self.zero_sensor is not None:
            self.zero_sensor.frame_callback(frame)

    def stop(self):
        """Stop the motor"""
        if self.zero_sensor is not None:
            self.zero_sensor.reset()








    

    
