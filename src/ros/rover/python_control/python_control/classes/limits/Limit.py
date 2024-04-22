#!/usr/bin/env python3

import abc
import jcan
from python_control.classes.sensors import Sensor
from typing import TypeVar

T = TypeVar('T')

class Limit():
    """Class to represent a limit"""
    def __init__(self, sensor: Sensor[T] = None):
        self.limit_hit = False # type: bool
        self.sensor = sensor # type: Sensor[T]
        self.bus = jcan.Bus() # type: jcan.Bus
        self.setup_can_bus()

    def setup_can_bus(self):
        """Setup the CAN bus"""
        self.bus.set_id_filter_mask(self.sensor.get_frame_id(), 0xFFF)
        self.bus.add_callback(self.sensor.get_frame_id(), self.frame_callback)
        self.bus.open(self.sensor.get_can_bus())
        self.create_timer(0.01, self.bus.spin)

    def update_limit_hit(self, limit_hit: bool):
        """Update the limit hit"""
        self.limit_hit = limit_hit
    
    def get_limit_hit(self):
        """Get the limit hit"""
        return self.limit_hit
    
    @abc.abstractmethod
    def frame_callback(self, frame: jcan.Frame):
        pass
