#!/usr/bin/env python3

import abc
import jcan
from python_control.classes.sensors import Sensor
from typing import TypeVar

T = TypeVar('T')

class Limit(abc.ABC):
    """Class to represent a limit"""
    def __init__(self, bus: jcan.Bus, sensor: Sensor[T] = None):
        self.limit_hit = False # type: bool
        self.sensor = sensor # type: Sensor[T]
        self.bus = bus
        self.setup_can_bus()

    def setup_can_bus(self):
        """Setup the CAN bus"""
        self.bus.add_callback(self.sensor.get_frame_id(), self.frame_callback)


    def update_limit_hit(self, limit_hit: bool):
        """Update the limit hit"""
        self.limit_hit = limit_hit
    
    def get_limit_hit(self):
        """Get the limit hit"""
        return self.limit_hit
    
    @abc.abstractmethod
    def frame_callback(self, frame: jcan.Frame):
        pass
