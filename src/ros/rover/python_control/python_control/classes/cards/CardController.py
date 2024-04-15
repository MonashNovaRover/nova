#!/usr/bin/env python3
import abc
import jcan
from python_control.classes.cards.Card import Card
from python_control.classes.controls.OneAxisControl import OneAxisControl

class CardController():
    """Class to interface with the cards on the CAN bus"""
    def __init__(self, card: Card, max_value: int, card_id: hex, control: OneAxisControl):
        self.card = card # type: Card
        self.max_value = max_value # type: int
        self.card_id = card_id # type: hex
        self.control = control # type: OneAxisControl

    def get_card(self):
        """Get the card type"""
        return self.card

    def get_max_value(self):
        """Get the max value of the card"""
        return self.max_value
    
    @abc.abstractmethod
    def get_frame(self) -> jcan.Frame:
        pass