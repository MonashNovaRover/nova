#!/usr/bin/env python3

import jcan
from python_control.classes.sensors.Sensor import Sensor

class TOFSensor(Sensor[int]):
    """Class to represent a time of flight sensor"""
    def __init__(self, frame_id: hex):
        super().__init__(frame_id=frame_id)
    
    def frame_callback(self, frame: jcan.Frame):
        """Update the sensor value based on the frame"""
        if frame.id != self.frame_id:
            raise ValueError("Invalid frame id")
        if len(frame.data) != 2:
            raise ValueError("Invalid frame data")
        
        self.set_sensor_value(int(frame.data[1] + (frame.data[0] << 8)))