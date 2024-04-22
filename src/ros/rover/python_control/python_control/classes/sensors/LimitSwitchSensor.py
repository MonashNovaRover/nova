#!/usr/bin/env python3

from python_control.classes.sensors.Sensor import Sensor
import jcan
from enum import Enum

class LimitSwitchValue(Enum):
    """Enum for the different limits of the motors"""
    HIT = 0xFF
    CLEAR = 0x00

class LimitSwitchSensor(Sensor[bool]):
    """Class to represent a limit switch sensor"""
    def __init__(self, can_bus: str, frame_id: hex, command_id: hex):
        super().__init__(can_bus=can_bus, frame_id=frame_id)
        self.command_id = command_id # type: hex

    def frame_callback(self, frame: jcan.Frame):
        """Update the sensor value based on the frame"""
        if frame.id != self.frame_id or frame.data[0] != self.command_id:
            raise ValueError("Invalid frame id")
        
        self.set_sensor_value(frame.data[1] == LimitSwitchValue.HIT.value)