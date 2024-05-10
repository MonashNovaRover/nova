#!/usr/bin/env python3

from logging import Logger
from python_control.controls.Control import Control

class ToggleControl(Control):
    """Class to control a toggle control"""

    def __init__(self, logger: Logger, on: bool = False):
        super().__init__(logger=logger)
        self.on = on # type: bool

    def start(self):
        """Start the toggle"""
        self.on = True

    def stop(self):
        """Stop the toggle"""
        self.on = False

    def toggle(self):
        """Toggle the toggle"""
        self.on = not self.on

    def is_on(self):
        """Check if the toggle is on"""
        return self.on
    

    
