#!/usr/bin/env python3

import abc
import jcan
from typing import TypeVar, Generic

T = TypeVar('T')

class Sensor(Generic[T]):
    """Class to represent a sensor"""
    def __init__(self, can_bus: str, frame_id: hex, initial_value: T = None, run_can: bool = True):
        self.can_bus = can_bus # type: str
        self.frame_id = frame_id # type: hex
        self.sensor_value = initial_value # type: T
        if run_can:
            self.bus = jcan.Bus() # type: jcan.Bus
            self.setup_can_bus()

    def setup_can_bus(self):
        """Setup the CAN bus"""
        self.bus.set_id_filter_mask(self.frame_id, 0xFFF)
        self.bus.add_callback(self.frame_id, self.frame_callback)
        self.bus.open(self.can_bus)
        self.create_timer(0.01, self.bus.spin)

    def set_sensor_value(self, sensor_value: T):
        """Set the sensor value"""
        self.sensor_value = sensor_value

    def get_sensor_value(self) -> T:
        """Get the sensor value"""
        return self.sensor_value
    
    def get_frame_id(self) -> hex:
        return self.frame_id
    
    def get_can_bus(self) -> str:
        return self.can_bus
    
    @abc.abstractmethod
    def publish_sensor(self):
        pass

    @abc.abstractmethod
    def frame_callback(self, frame: jcan.Frame):
        pass