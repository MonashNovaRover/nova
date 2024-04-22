#!/usr/bin/env python3
import abc
import jcan
from python_control.classes.cards.Card import Card
from python_control.classes.controls.OneAxisControl import OneAxisControl

class CardController():
    """Class to interface with the cards on the CAN bus"""
    def __init__(self, card: Card, max_value: int, can_bus: str, card_id: hex, control: OneAxisControl):
        self.card = card # type: Card
        self.max_value = max_value # type: int
        self.can_bus = can_bus # type: str
        self.card_id = card_id # type: hex
        self.control = control # type: OneAxisControl
 
        self.bus = jcan.Bus() # type: jcan.Bus
        self.start_can()

    def start_can(self):
        self.bus.open(self.can_bus)
        self.create_timer(0.01, self.bus.spin)
        self.create_timer(0.05, self.control_send_callback)

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