#!/usr/bin/env python3

import abc
import jcan
from typing import TypeVar, Generic
from logging import Logger

T = TypeVar('T')

class Sensor(abc.ABC, Generic[T]):
    """Class to represent a sensor"""
    def __init__(self, bus: jcan.Bus, logger: Logger, frame_id: hex, initial_value: T = None, run_can: bool = True):
        self.frame_id = frame_id # type: hex
        self.sensor_value = initial_value # type: T
        self.bus = bus
        self.logger = logger
        if run_can:
            self.setup_can_bus()

    def get_logger(self):
        return self.logger

    def setup_can_bus(self):
        """Setup the CAN bus"""
        self.bus.add_callback(self.frame_id, self.frame_callback)

    def set_sensor_value(self, sensor_value: T):
        """Set the sensor value"""
        self.sensor_value = sensor_value

    def get_sensor_value(self) -> T:
        """Get the sensor value"""
        return self.sensor_value
    
    def get_frame_id(self) -> hex:
        return self.frame_id
    
    @abc.abstractmethod
    def publish_sensor(self):
        pass

    @abc.abstractmethod
    def frame_callback(self, frame: jcan.Frame):
        pass