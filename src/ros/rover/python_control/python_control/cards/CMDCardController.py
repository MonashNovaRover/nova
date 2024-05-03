#!/usr/bin/env python3
from logging import Logger
from struct import pack
import jcan
from python_control.cards.Card import Card
from python_control.cards.CardController import CardController
from python_control.controls.OneAxisControl import OneAxisControl


class CMDCardController(CardController):
    """Class to control the CMD card on the CAN bus"""
    def __init__(self, bus: jcan.Bus, logger: Logger, card_id: hex, control: OneAxisControl):
        super().__init__(card=Card.CMD, max_value=32767, card_id=card_id, control=control, bus=bus, logger=logger)

    def get_frame(self) -> jcan.Frame:
        """Get the frame to send over the CAN bus"""

        # Set the data based on the direction, velocity, max value, and max percent
        data = int(self.control.get_direction().value * self.control.get_velocity() * self.get_max_value() * self.control.get_max_percent())

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if data > self.get_max_value():
            data = self.get_max_value()

        # Pack the data into a list
        packed_data = list(pack('>h', int(data)))

        # Create and return the frame
        frame = jcan.Frame(id=self.card_id, data=packed_data)

        return frame