__package__ = "autonomous"

from controller.ControllerInterface import ControllerInterface
from controller.YawStarController import YawStarController
from math_utils.controller_math import *
from config.runtime_params import dist_through_gate_m
import numpy as np

class GateController(ControllerInterface):
    def __init__(self, logger):
        self.logger = logger
        self.state = None
        self.target_waypoint = None
        self.controller = YawStarController()
        self.gate = None

    def get_drive_command(self, target_waypoint, state: State, goal: Tuple(float), gate: Tuple(Tuple(float))):
        self.state = state
        self.gate = gate
        if self.target_waypoint is None:
            self.calculate_target()

        return self.controller.get_drive_command(self.target_waypoint, self.state)

    def calculate_target(self):
        gate_l, gate_r = np.array(gate[0]), np.array(gate[1])
        gate_mid = 0.5 * (gate_l + gate_r)

        perp_to_gate = np.array([(gate_r - gate_l)[1], (gate_l - gate_r)[0]])
        orientation_vector = np.array([np.cos(self.state.yaw), np.sin(self.state.yaw)])
        direction = np.sign(np.dot(orientation_vector, perp_to_gate))

        centre_of_gate_to_target = direction * dist_through_gate_m * perp_to_gate / magnitude(perp_to_gate)

        self.target_waypoint = gate_mid + centre_of_gate_to_target
