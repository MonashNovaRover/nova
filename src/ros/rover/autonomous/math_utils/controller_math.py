__package__ = "autonomous"
"""
Controller math is a set of pure functions (meaning they only take immutable inputs and return immutable outputs,
with no side effects (such as changing any external mutable variables, global variables etc.)

The benefit of using as many functions like this as possible is that we can better reason about and mathematically
assert the behaviour of functions, compositions of functions, and as a result, entire systems.
"""

import config.runtime_params as runtime_params
import math
import numpy as np


class State:
    """
    Represents a state
    """

    def __init__(self, x=0.0, y=0.0, yaw=0.0, velocity=0.0, angular_velocity=0.0):
        self.x = x
        self.y = y
        self.yaw = yaw
        self.velocity = velocity
        self.angular_velocity = angular_velocity

def tank_turn_target_yaw_rate(yaw_diff):
    """
    Calculates target yaw rate. Uses sin function so that we smoothly speed up and slow down in order to change yaw
    Domain: This function should have inputs such that:
        abs(current_yaw - target_yaw) is within pi
    Range:
        [-max_yaw_rate, -min_yaw_rate], [min_yaw_rate, max_yaw_rate], [0, 0]
    """
    # print("target yaw: " + str(target_yaw) + " | current_yaw: " + str(current_yaw))
    return -np.sign(yaw_diff)

def yaw_difference(facing, target):
    """
    Returns the signed minimum yaw required to rotate from one orientation vector to another. A positive angle refers to traveling
    clockwise (in a left-handed coordinate system). Use cases: this function is used to determine the absolute value of the yaw difference, to determine
    if we should travel there. It is also used to determine how far we have to turn, so we can calculate an appropriate
    speed.
    :param: facing - orientation vector of the rover currently
    :param: target - target orientation of the rover
    Range: between -pi and pi
    """

    cross = np.cross(facing, target)
    dot = np.dot(facing, target)
    norms = np.sqrt(np.dot(facing, facing)) * np.sqrt(np.dot(target, target))
    # using dot product to find minimum angle
    theta = np.arccos(np.round(dot/norms, 8))

    yaw_sign = np.sign(cross[2]) if np.round(dot, 5) != -1. else 1

    diff = yaw_sign * theta
    assert -np.pi < diff <= np.pi
    return diff

def vector_argument(vector):
    """
    Returns the argument of a vector as an angle from -pi to pi
    """

    return np.arctan2(vector[1], vector[0])



def distance(current, target):
    return math.sqrt(((current[0] - target[0]) ** 2.0) + ((current[1] - target[1]) ** 2.0))


def crow_fly_target_velocity(current, target):
    """
    :current: 2tuple
    :target: 2tuple
    returns: range: 0, or within [min_speed, max_speed]
    """

    return .1 
