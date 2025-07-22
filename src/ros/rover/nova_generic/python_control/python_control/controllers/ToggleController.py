#!/usr/bin/env python3

from logging import Logger
import jcan
from python_control.controllers.Card import Card
from python_control.controllers.Controller import Controller
from python_control.controls.ToggleControl import ToggleControl

class ToggleController(Controller):
    """Class to control the JONO card on the CAN bus"""
    def __init__(self, frame_id: hex, control: ToggleControl, bus: jcan.Bus, logger: Logger, toggle_command_on: hex, control_id: hex = None, toggle_command_off: hex = None):
        super().__init__(card=Card.TOGGLE, max_value=255, frame_id=frame_id, control=control, bus=bus, logger=logger)
        # Command Ids = 1 Byte, (0x00 - 0xFF)4
        self.control_id = control_id # type: hex
        self.toggle_command_on = toggle_command_on # type: hex
        self.toggle_command_off = toggle_command_off # type: hex

    def get_frame(self) -> jcan.Frame:
        """Get the frame to send over the CAN bus"""
        control: ToggleControl = self.get_control()

        # Set the command based on the direction
        command: hex = None
        if not control.is_on() and self.toggle_command_off is not None:
            command = self.toggle_command_off
        elif control.is_on() and self.toggle_command_on is not None:
            command = self.toggle_command_on

        if command is None:
            return None

        # Pack the data into a list
        packed_data : hex = None
        if self.control_id is not None:
            packed_data = [self.control_id, command]
        else:
            packed_data = [command]

        # Create and return the frame
        frame = jcan.Frame(id=self.frame_id, data=packed_data)

        return frame
