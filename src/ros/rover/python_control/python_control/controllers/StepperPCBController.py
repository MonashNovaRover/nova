#!/usr/bin/env python3
from logging import Logger
from struct import pack
import jcan
from python_control.controllers.Card import Card
from python_control.controllers.Controller import Controller
from python_control.controls.OneAxisPositionControl import OneAxisPositionControl

class StepperPCBPositionController(Controller):
    """Class to control the CMD card on the CAN bus"""
    def __init__(self, bus: jcan.Bus, logger: Logger, card_id: hex, command_id: hex, control: OneAxisPositionControl):
        super().__init__(card=Card.STEPPER_PCB, max_value=32767, card_id=card_id, control=control, bus=bus, logger=logger)
        # Command Id = 1 Byte, (0x00 - 0xFF)
        self.command_id = command_id # type: hex

    def get_frame(self) -> jcan.Frame:
        """Get the frame to send over the CAN bus"""
        control: OneAxisPositionControl = self.get_control()

        # Set the data based on the position
        data = int(control.get_position())

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if data > self.get_max_value() or data < -self.get_max_value():
            data = self.get_max_value()

        # Pack the data into a list
        packed_data = [int(self.command_id)] + list(pack('>h', int(data)))

        # Create and return the frame
        frame = jcan.Frame(id=self.card_id, data=packed_data)

        return frame

    def control_send_callback(self):
        if not self.is_stopped():
            super().control_send_callback()
        else:
            self.get_logger().debug("Controller is stopped, not sending frame")
        
        