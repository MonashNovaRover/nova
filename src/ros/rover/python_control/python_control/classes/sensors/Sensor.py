#!/usr/bin/env python3

import abc
import jcan
from typing import TypeVar, Generic

T = TypeVar('T')

class Sensor(Generic[T]):
    """Class to represent a sensor"""
    def __init__(self, frame_id: hex, initial_value: T = None):
        self.frame_id = frame_id # type: hex
        self.sensor_value = initial_value # type: T

    def set_sensor_value(self, sensor_value: T):
        """Set the sensor value"""
        self.sensor_value = sensor_value

    def get_sensor_value(self) -> T:
        """Get the sensor value"""
        return self.sensor_value
    
    @abc.abstractmethod
    def publish_sensor(self):
        pass

    @abc.abstractmethod
    def frame_callback(self, frame: jcan.Frame):
        pass