#!/usr/bin/env python3

from python_control.sensors.Sensor import Sensor
from logging import Logger
import jcan

class CommandSensor(Sensor[bool]):
    """Class to represent a limit switch sensor"""
    def __init__(
            self, 
            bus: jcan.Bus, 
            logger: Logger, 
            frame_id: hex, 
            command_id: hex,
            state_id: hex = None,
            initial_value: bool = False,
            run_can: bool = True,
        ):
        super().__init__(
            bus=bus, 
            logger=logger, 
            frame_id=frame_id, 
            initial_value=initial_value,
            run_can=run_can
        )
        # Command ID = 1 Byte, (0x00 - 0xFF)
        self.command_id = command_id # type: hex
        self.state_id = state_id

    def callback_function(self, frame: jcan.Frame):
        """Update the sensor value based on the frame"""
        if frame.data[0] == self.command_id:
            if self.state_id is not None:
                if len(frame.data) != 2:
                    self.get_logger().error("Invalid frame data length")
                    return
                elif frame.data[1] == self.state_id:
                    self.set_sensor_value(True)
            else:
                self.set_sensor_value(True)
        

    def reset(self):
        self.set_sensor_value(False)

    
    def publish_sensor(self):
        pass