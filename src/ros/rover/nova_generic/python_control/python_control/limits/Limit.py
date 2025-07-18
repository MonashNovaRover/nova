#!/usr/bin/env python3

import abc
from logging import Logger
import jcan
from python_control.sensors import Sensor
from typing import TypeVar

T = TypeVar('T')

class Limit(abc.ABC):
    """Class to represent a limit"""
    def __init__(self, bus: jcan.Bus, logger: Logger, sensor: Sensor = None):
        self.limit_hit = False # type: bool
        self.sensor = sensor # type: Sensor[T]
        self.bus = bus # type: jcan.Bus
        self.logger = logger # type: Logger
        self.setup_can_bus()

    def get_logger(self):
        return self.logger

    def setup_can_bus(self):
        """Setup the CAN bus"""
        self.bus.add_callback(self.sensor.get_frame_id(), self.frame_callback)


    def update_limit_hit(self, limit_hit: bool):
        """Update the limit hit"""
        self.limit_hit = limit_hit
        if self.limit_hit:
            self.get_logger().debug("Limit hit")
    
    def get_limit_hit(self):
        """Get the limit hit"""
        return self.limit_hit
    
    @abc.abstractmethod
    def frame_callback(self, frame: jcan.Frame):
        pass
