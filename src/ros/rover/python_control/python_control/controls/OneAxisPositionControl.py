from logging import Logger
from python_control.controls.Control import Control
from python_control.sensors import IntegerSensor

class OneAxisPositionControl(Control):
    """Class to control a single axis motor"""
    ZERO = "zero"

    def __init__(self, logger: Logger, positions: dict[str, int] = {}, position_sensor: IntegerSensor = None):
        super().__init__(logger=logger)
        positions[self.ZERO] = 0
        self.position_name = self.ZERO
        self.positions = positions
        self.position_sensor = position_sensor

    def get_goal_position(self):
        """Get the position to move the motor to"""
        if self.position_name is None:
            return self.position_sensor.get_sensor_value()
        return self.positions[self.position_name]
    
    def get_position_name(self):
        """Get the name of the current position to move the motor to"""
        return self.position_name
    
    def get_positions(self):
        """Get the set of available positions of the motor"""
        return self.positions
    
    def get_current_position(self):
        """Get the current position of the motor"""
        return self.position_sensor.get_sensor_value()

    def add_position(self, name: str, position: int):
        """Add a position to the set of available positions"""
        self.positions[name] = position
    
    def remove_position(self, name: str):
        """Remove a position from the set of available positions"""
        del self.positions[name]

    def go_to_position(self, name: str):
        """Go to a specific position"""
        self.position_name = name
    
    def valid_goal(self, name: str):
        """Check if the motor is at the goal position"""
        return name in self.positions.keys()
    
    def zero(self):
        """Move the motor to the zero position"""
        self.position_name = self.ZERO

    def distance_to_position(self) -> int:
        """Get the distance to the set position"""
        return abs(self.positions[self.position_name] - self.position_sensor.get_sensor_value())

    def is_at_position(self) -> bool:
        """Check if the motor is at the specific position"""
        return self.position_sensor.get_sensor_value() == self.positions[self.position_name]

    def stop(self):
        """Stop the motor"""
        pass









    

    
