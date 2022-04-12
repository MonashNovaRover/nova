__package__ = "autonomous"

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This script defines an interface for different types
of controllers that define the movement of the rover
through various stages of the URC autonomous task. A
controller simply takes in information about the state
of the Rover and the map and returns a drive command
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: This class is purely logical and doesn't need to be
        a ROS node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        autonomous
AUTHOR(S):      Max Tory
CREATION:       12/04/2022
EDITED:         12/04/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from abc import *
from math_utils.controller_math import State

class ControllerInterface(ABC):
    @abstract_method
    def get_drive_command(self, target_waypoint, state: State, goal: Tuple(float), gate: Tuple(Tuple(float))):
        """
        Gets the drive command based on the type of controller we are using
        :param target_waypoint: the next waypoint in the list we are aiming for
        :param state: the current state (pitch, roll, yaw, x, y, z) of the rover
        :param goal: the end goal of the rover
        :param gate: the position of the two poles of the gate, if they have been detected
        """
        pass

    def log(self, drive):
        if drive['steer']:
            Controller.log_update("Controller Steering", self.target_waypoint, yaw_diff,
                                  distance((self.state.x, self.state.y), self.target_waypoint))

        else:
            Controller.log_update("Controller Driving", self.target_waypoint, yaw_diff,
                                  distance((self.state.x, self.state.y), self.target_waypoint))