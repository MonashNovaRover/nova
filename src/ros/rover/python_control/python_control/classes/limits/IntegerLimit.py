#!/usr/bin/env python3
import jcan
<<<<<<<< HEAD:python_control/python_control/classes/limits/TOFLimit.py
from python_control.classes.limits.Limit import Limit
from python_control.classes.sensors.TOFSensor import TOFSensor
========
from control.classes.limits.Limit import Limit
from control.control.classes.sensors.IntegerSensor import IntegerSensor
>>>>>>>> e79d120e (bus injection, genericise TOF, some other fixes):python_control/python_control/classes/limits/IntegerLimit.py


class IntegerLimit(Limit):
    """Class to represent a time of flight sensor"""
    def __init__(self, bus: jcan.Bus, maximum: bool, limit_value: int, integer_sensor: IntegerSensor):
        super().__init__(bus=bus, sensor=integer_sensor)
        self.maximum = maximum  # [True for max, False for min]
        self.limit_value = limit_value  # type: int

    def frame_callback(self, frame: jcan.Frame):
        """Update the limit hit based on the frame"""
        self.sensor.frame_callback(frame)
        sensor_value = self.sensor.get_sensor_value()

        if self.maximum:
            self.update_limit_hit(sensor_value >= self.limit_value)
        else:
            self.update_limit_hit(sensor_value <= self.limit_value)