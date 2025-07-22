#!/usr/bin/env python3

from logging import Logger
import jcan
from python_control.limits.Limit import Limit
from python_control.sensors.Sensor import Sensor

class LimitSwitchLimit(Limit):
    """Class to represent a limit switch"""
    def __init__(self, bus: jcan.Bus, logger: Logger, limit_switch: Sensor[bool]):
        super().__init__(bus=bus, logger=logger, sensor=limit_switch)

    def frame_callback(self, frame: jcan.Frame):
        self.sensor.frame_callback(frame)
        self.update_limit_hit(bool(self.sensor.get_sensor_value()))