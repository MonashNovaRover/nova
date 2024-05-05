#!/usr/bin/env python3
import abc
from logging import Logger
import jcan
from python_control.controllers.Card import Card
from python_control.controls.Control import Control

class Controller(abc.ABC):
    """Class to interface with the cards on the CAN bus"""
    def __init__(self, card: Card, max_value: int, card_id: hex, control: Control, bus: jcan.Bus, logger: Logger):
        self.card = card # type: Card
        self.max_value = max_value # type: int
        # Card Id = 12 bits, (0x000 - 0xFFF)
        self.card_id = card_id # type: hex
        self.control = control # type: Control
        self.bus = bus # type: jcan.Bus
        self.logger = logger
        self.stopped = False

    def get_logger(self) -> Logger:
        return self.logger
    
    def get_card(self) -> Card:
        """Get the card type"""
        return self.card
    
    def get_control(self) -> Control:
        """"""
        return self.control
    
    def is_stopped(self) -> bool:
        """Get the stop status of the controller"""
        return self.stopped

    def get_max_value(self) -> int:
        """Get the max value of the card"""
        return self.max_value
    
    def control_send_callback(self) -> None:
        frame = self.get_frame()
        self.get_logger().debug("Sending frame: %s", frame)
        self.bus.send(frame)
    
    @abc.abstractmethod
    def get_frame(self) -> jcan.Frame:
        pass

    def stop(self) -> None:
        """Stop the controller"""
        self.control.stop()
        self.stopped = True