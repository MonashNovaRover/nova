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
from autonomous.config.runtime_params import straight_drive_fraction, spin_achieved_delta
from typing import Tuple


def tank_turn_target_yaw_rate(yaw_diff: float) -> float:
    """
    Calculates target steer value (mapping to -1:1 controller right stick position)
    Domain: This function should have inputs such that:
        abs(yaw_diff) <= pi
    Range:
        [-1, 1]

    :param yaw_diff: difference between current and desired yaw
    """
    max_non_tank = np.pi/4
    turn_frac = min(abs(yaw_diff) / max_non_tank, 1)
    return -np.sign(yaw_diff) * (turn_frac**2)


def steer_val_to_radius(steer: float) -> float:
    """
    Calculate the turn radius associated with a steer value
    :param steer: the steer value from [-1, 1]
    :return: the turn radius in meters
    """
    if steer == 0:
        # When we negate -inf in the steer val to wheel angles function, this will become positive
        return -np.inf
    else:
        return 1 / steer - np.sign(steer)

def steer_val_to_wheel_angles(steer: float) -> float:
    """
    Calculate left and right wheel angles for a given steer value. Used to determine wheel angle
    errors so we can work out how long it will take the wheels to get into position.
    :param steer: the steer value from [-1, 1]
    :return: the left and right wheel angles in radians
    """
    ROVER_LEN_2 = 0.6
    ROVER_WIDTH_2 = 0.5
    radius = steer_val_to_radius(steer)
    left_angle = np.arctan2(ROVER_LEN_2, -(radius + ROVER_WIDTH_2))
    right_angle = np.arctan2(ROVER_LEN_2, -(radius - ROVER_WIDTH_2))
    return left_angle, right_angle


def drive_speed_from_turning_error(target_steer, current_steer) -> float:
    """
    :param target_steer: the steer value from [-1, 1] we would like to turn at
    :param current_steer: the current steer value from [-1, 1] the wheels are oriented at
    :return: the speed we will drive at given the error of our wheel orientations
    """
    MAX_STEER_ERROR = np.pi/4
    # left and right desired wheel angles
    target_left, target_right = steer_val_to_wheel_angles(target_steer)
    # left and right true wheel angles
    current_left, current_right = steer_val_to_wheel_angles(current_steer)
    # maximum error between desired and true wheel angles
    steer_error_radians = max(abs(target_left - current_left), abs(target_right - current_right))
    scaled_steer_error = min(steer_error_radians / MAX_STEER_ERROR, 1)
    drive_rate = straight_drive_fraction * (1 - scaled_steer_error**2)
    return drive_rate


def tank_turn_target_drive_rate(yaw_diff: float) -> float:
    """
    Calculates target drive rate. 
    Domain: This function should have inputs such that:
        abs(current_yaw - target_yaw) is within pi
    Range:
        [0, max_drive_rate]

    :param yaw_diff: difference between current and desired yaw
    :return: float
    """
    max_non_tank = np.pi/4
    drive_frac = min(abs(yaw_diff) / max_non_tank, 1)
    half_max_speed = straight_drive_fraction / 2
    return half_max_speed * drive_frac + half_max_speed


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
    scaled_dot = dot / norms
    # using dot product to find minimum angle
    theta = np.arccos(np.round(scaled_dot, 8))

    yaw_sign = np.sign(cross[2]) if np.round(scaled_dot, 5) != -1. else 1

    diff = yaw_sign * theta
    assert -np.pi < diff <= np.pi
    return diff


def spin_achieved(direction: int, facing: np.array, target: np.array):
    delta = direction * yaw_difference(facing, target)

    return spin_achieved_delta / 2 < delta < spin_achieved_delta


def interpolate_circle_points(centre: Tuple, num_points: int = 8, radius: int = 10):
    d_theta = 2 * np.pi / num_points
    theta = 0
    pts = []
    for _ in range(num_points):
        pts.append((radius * np.cos(theta) + centre[0], radius * np.sin(theta) + centre[1]))
        theta += d_theta

    return pts


def average_vector(vectors: list) -> np.array:
    """
    Takes a list of vectors and returns their average
    :param vectors: numpy array
    """
    return np.mean(np.array(vectors), axis=1)


def vector_argument(vector: np.array) -> np.array:
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
    if target is None: return 0.0
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


def magnitude(vector):
    """
    Magnitude of a vector from 2-norm
    """
    return np.dot(vector, vector) ** 0.5
