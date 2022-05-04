__package__ = "autonomous"

from controller.ControllerInterface import ControllerInterface
from controller.DriveController import DriveController
from math_utils.controller_math import *
from config.runtime_params import dist_through_gate_m
from typing import Tuple
import numpy as np

class GateController(ControllerInterface):
    def __init__(self, logger):
        self.logger = logger
        self.state = None
        self.target_waypoint = None
        self.controller = DriveController()
        self.gate = None

    def get_drive_command(self, target_waypoint, state: State, goal: Tuple[float], gate: Tuple[Tuple[float]]):
        self.state = state
        self.gate = gate
        if self.target_waypoint is None:
            self.calculate_target()

        return self.controller.get_drive_command(self.target_waypoint, self.state)

    def calculate_target(self):
        gate_l, gate_r = np.array(self.gate[0]), np.array(self.gate[1])
        gate_mid = 0.5 * (gate_l + gate_r)

        # negative reciprocal gives perpendicular vector to the vector between the gate
        perp_to_gate = np.array([(gate_r - gate_l)[1], (gate_l - gate_r)[0]])
        # Current yaw as vector
        orientation_vector = np.array([np.cos(self.state.yaw), np.sin(self.state.yaw)])
        # -1 if we are facing away from perp vector, +1 if we are towards it
        direction = np.sign(np.dot(orientation_vector, perp_to_gate))

        # Vector from the middle of the two gate poles to our target
        centre_of_gate_to_target = direction * dist_through_gate_m * perp_to_gate / magnitude(perp_to_gate)

        self.target_waypoint = gate_mid + centre_of_gate_to_target

    def completed(self) -> bool:
        return self.controller.completed()
