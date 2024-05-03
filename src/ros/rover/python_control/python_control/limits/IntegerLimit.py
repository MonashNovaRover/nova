#!/usr/bin/env python3
from logging import Logger
import jcan

from python_control.limits.Limit import Limit
from python_control.limits.Limit import Limit
from python_control.sensors.IntegerSensor import IntegerSensor

class IntegerLimit(Limit):
    """Class to represent a time of flight sensor"""
    def __init__(self, bus: jcan.Bus, logger: Logger, is_maximum: bool, limit_value: int, integer_sensor: IntegerSensor):
        super().__init__(bus=bus, logger=logger, sensor=integer_sensor)
        self.is_maximum = is_maximum  # [True for max, False for min]
        self.limit_value = limit_value  # type: int

    def frame_callback(self, frame: jcan.Frame):
        """Update the limit hit based on the frame"""
        self.sensor.frame_callback(frame)
        sensor_value = self.sensor.get_sensor_value()

        if self.is_maximum:
            self.update_limit_hit(sensor_value >= self.limit_value)
        else:
            self.update_limit_hit(sensor_value <= self.limit_value)