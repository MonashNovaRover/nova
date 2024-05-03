#!/usr/bin/env python3
import abc
from logging import Logger
import jcan
from python_control.cards.Card import Card
from python_control.controls.OneAxisControl import OneAxisControl

class CardController(abc.ABC):
    """Class to interface with the cards on the CAN bus"""
    def __init__(self, card: Card, max_value: int, card_id: hex, control: OneAxisControl, bus: jcan.Bus, logger: Logger):
        self.card = card # type: Card
        self.max_value = max_value # type: int
        self.card_id = card_id # type: hex
        self.control = control # type: OneAxisControl
        self.bus = bus # type: jcan.Bus
        self.logger = logger

    def get_card(self):
        """Get the card type"""
        return self.card
    
    def get_control(self):
        """"""
        return self.control

    def get_max_value(self):
        """Get the max value of the card"""
        return self.max_value
    
    def control_send_callback(self):
        frame = self.get_frame()
        self.bus.send(frame)
    
    @abc.abstractmethod
    def get_frame(self) -> jcan.Frame:
        pass