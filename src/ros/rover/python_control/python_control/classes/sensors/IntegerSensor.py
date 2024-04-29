#!/usr/bin/env python3

from logging import Logger
import jcan
from python_control.classes.sensors.Sensor import Sensor

class IntegerSensor(Sensor[int]):
    """Class to represent a time of flight sensor"""
    def __init__(self, bus: jcan.Bus, logger: Logger, frame_id: hex):
        super().__init__(bus=bus, logger=logger, frame_id=frame_id)
    
    def frame_callback(self, frame: jcan.Frame):
        """Update the sensor value based on the frame"""
        if frame.id != self.frame_id:
            raise ValueError("Invalid frame id")
        if len(frame.data) != 2:
            raise ValueError("Invalid frame data")
        
        self.set_sensor_value(int(frame.data[1] + (frame.data[0] << 8)))

    def publish_sensor(self):
        pass