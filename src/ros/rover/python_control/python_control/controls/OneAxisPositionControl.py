from logging import Logger
from python_control.controls.Control import Control

class OneAxisPositionControl(Control):
    """Class to control a single axis motor"""

    def __init__(self, logger: Logger, position: int = 0, positions: dict[str, int] = {}):
        super().__init__(logger=logger)
        self.position = position
        self.positions = positions

    def get_position(self):
        """Get the position of the motor"""
        return self.position
    
    def get_positions(self):
        """Get the set of available positions of the motor"""
        return self.positions

    def add_position(self, name: str, position: int):
        """Add a position to the set of available positions"""
        self.positions[name] = position
    
    def remove_position(self, name: str):
        """Remove a position from the set of available positions"""
        del self.positions[name]

    def go_to_position(self, name: str):
        """Go to a specific position"""
        self.position = self.positions[name]

    def stop(self):
        """Stop the motor"""
        pass









    

    
