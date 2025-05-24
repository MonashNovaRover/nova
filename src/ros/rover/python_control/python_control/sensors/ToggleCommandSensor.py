#!/usr/bin/env python3

from python_control.sensors.Sensor import Sensor
from rclpy.publisher import Publisher
from std_msgs.msg import Bool
from logging import Logger
import jcan

class ToggleCommandSensor(Sensor[bool]):
    """Class to represent a limit switch or hall effect sensor"""
    def __init__(
            self, 
            bus: jcan.Bus, 
            logger: Logger, 
            frame_id: hex, 
            state_id_on: hex,
            state_id_off: hex,
            control_id: hex = None,
            initial_value: bool = False,
            run_can: bool = True,
            publisher: Publisher = None,
        ):
        super().__init__(
            bus=bus, 
            logger=logger, 
            frame_id=frame_id, 
            initial_value=initial_value,
            run_can=run_can,
            publisher=publisher,
        )
        # Command ID = 1 Byte, (0x00 - 0xFF)
        self.control_id = control_id # type: hex
        self.state_id_on = state_id_on # type: hex
        self.state_id_off = state_id_off # type: hex

    def callback_function(self, frame: jcan.Frame):
        """Update the sensor value based on the frame"""
        state : hex = None
        if self.control_id is not None:
            if len(frame.data) == 1 or frame.data[0] != self.control_id:
                return
            state = frame.data[1]            
        else:
            state = frame.data[0]

        if state == self.state_id_on:
            self.set_sensor_value(True)
        elif state == self.state_id_off:
            self.set_sensor_value(False)                

    def reset(self):
        self.set_sensor_value(False)

    
    def publish_sensor(self):
        msg = Bool()
        msg.data = self.get_sensor_value()
        self.publisher.publish(msg)