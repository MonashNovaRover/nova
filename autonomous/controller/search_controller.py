from controller.ControllerInterface import ControllerInterface
from typing import Tuple
from math_utils.controller_math import *

class SearchController(ControllerInterface):
    def __init__(self, is_gate: bool=False):
        self.found_one = False
        self.found_two = False
        self.is_gate = is_gate

        self.turned = false

    def get_drive_command(self, target_waypoint, state: State, goal: Tuple[float], gate: Tuple[Tuple[float]]):
        if not self.turned:
            self.turn()