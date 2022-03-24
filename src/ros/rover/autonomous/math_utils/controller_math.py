#!/usr/bin/python3

__package__ = "autonomous"

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Controller math is a set of pure functions (meaning they only take immutable inputs and return immutable outputs,
with no side effects (such as changing any external mutable variables, global variables etc.)

The benefit of using as many functions like this as possible is that we can better reason about and mathematically
assert the behaviour of functions, compositions of functions, and as a result, entire systems.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Note: This is not a ros node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: autonomous
AUTHOR(S): Max Tory, Liam Whittle
CREATION:	6/3/2021
EDITED:		7/3/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""


import math
import numpy as np
from config.runtime_params import straight_drive_fraction


class State:
    """
    Represents a state of the rover in two dimension space - i.e. it only has an (x, y) position and a yaw.
    For use as a container in other classes.
    """
    def __init__(self, x=0.0, y=0.0, yaw=0.0, velocity=0.0, angular_velocity=0.0):
        self.x = x
        self.y = y
        self.yaw = yaw
        self.velocity = velocity
        self.angular_velocity = angular_velocity


def tank_turn_target_yaw_rate(yaw_diff: float) -> float:
    """
    Calculates target yaw rate. Uses sin function so that we smoothly speed up and slow down in order to change yaw
    Domain: This function should have inputs such that:
        abs(current_yaw - target_yaw) is within pi
    Range:
        [-max_yaw_rate, -min_yaw_rate], [min_yaw_rate, max_yaw_rate], [0, 0]

    :param yaw_diff: difference between current and desired yaw
    :return: float
    """
    return -np.sign(yaw_diff)


def yaw_difference(facing: np.array, target: np.array) -> float:
    """
    Returns the signed minimum yaw required to rotate from one orientation vector to another. A positive angle refers
    to traveling clockwise (in a left-handed coordinate system). Use cases: this function is used to determine the
    absolute value of the yaw difference, to determine if we should travel there. It is also used to determine how
    far we have to turn, so we can calculate an appropriate speed.

    :param facing: orientation vector of the rover currently
    :param target: target orientation of the rover Range: between -pi and pi
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
    :param vector: array like with on dimension and two elements
    :return: the float vector argument as a radian
    """
    return np.arctan2(vector[1], vector[0])


def distance(current, target):
    """
    Calculates the pythagorean distance between two 1 dimensional array like objects
    :param current: array like with one dimension and two elements
    :param target: array like with one element and two dimensions
    :return: positive float value of the euclidean distance between the two points
    """
    if not target: return 0
    return math.sqrt(((current[0] - target[0]) ** 2.0) + ((current[1] - target[1]) ** 2.0))


def crow_fly_target_velocity(current, target):
    """
    This function is currently implemented as a constant, as we aren't intending to modify the speed of the rover
    based on how far it yet has to travel
    :param current: 2tuple
    :param target: 2tuple
    returns: 0, or within range [min_speed, max_speed]
    """
    return straight_drive_fraction 
