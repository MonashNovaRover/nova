#!/usr/bin/env python3

import jcan
from python_control.classes.limits.Limit import Limit
from python_control.classes.sensors.LimitSwitchSensor import LimitSwitchSensor

class LimitSwitchLimit(Limit):
    """Class to represent a limit switch"""
    def __init__(self, bus: jcan.Bus, limit_switch: LimitSwitchSensor):
        super().__init__(bus=bus, sensor=limit_switch)

    def frame_callback(self, frame: jcan.Frame):
        self.sensor.frame_callback(frame)
        self.update_limit_hit(bool(self.sensor.get_sensor_value()))