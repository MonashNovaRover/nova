"""
Controller math is a set of pure functions (meaning they only take immutable inputs and return immutable outputs,
with no side effects (such as changing any external mutable variables, global variables etc.)

The benefit of using as many functions like this as possible is that we can better reason about and mathematically
assert the behaviour of functions, compositions of functions, and as a result, entire systems.
"""

import controller_params
import math

import numpy as np
from numpy.linalg import norm


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


def tank_turn_target_yaw_rate(current_yaw, target_yaw):
    """
    Calculates target yaw rate. Uses sin function so that we smoothly speed up and slow down in order to change yaw
    Domain: This function should have inputs such that:
        abs(current_yaw - target_yaw) is within pi
    Range:
        [-max_yaw_rate, -min_yaw_rate], [min_yaw_rate, max_yaw_rate], [0, 0]
    """
    # print("target yaw: " + str(target_yaw) + " | current_yaw: " + str(current_yaw))
    d = yaw_difference(a=current_yaw, b=target_yaw)
    assert -math.pi <= d <= math.pi
    if d == 0.0:
        return 0
    sign = 1 if d > 0.0 else -1
    return sign * controller_params.max_yaw_rate


def desired_heading(start, end):
    """
    Function returns the total positive angle (between 0 and 2pi radians) between the positive y axis and the
    vector formed by the two start and end coordinates, heading counter clockwise. In other words, what COMPASS
    angle would we be heading from north if we needed to reach end from start.
    Domain: two 2-tuples (float, float), representing coordinates
    Range: 0 to 2pi radians
    """
    assert len(start) == 2
    assert len(end) == 2

    # the signed bearing is within [-pi, pi]
    signed_bearing = math.atan2(end[0] - start[0], end[1] - start[1])

    return signed_bearing if signed_bearing >= 0 else signed_bearing + 2 * math.pi


def yaw_difference(a, b):
    """
    Returns the signed minimum yaw required to travel to get to b from a, where a positive angle refers to traveling
    clockwise. Use cases: this function is used to determine the absolute value of the yaw difference, to determine
    if we should travel there. It is also used to determine how far we have to turn, so we can calculate an appropriate
    speed.
    :param: a: 2-tuple referring to the starting angle
    :param: b: 2-tuple referring to the desired angle
    Domain: a and be are within 0 and 2pi
    Range: between -pi and pi
    """

    a += math.pi * 2.0 if a < 0 else 0
    b += math.pi * 2.0 if b < 0 else 0

    assert 0.0 <= a <= math.pi * 2.0
    assert 0.0 <= b <= math.pi * 2.0

    # if desired way point has greater bearing
    if b > a:
        # go clockwise if that's shorter
        if b - a <= math.pi:
            d = b - a
        # else go anti clockwise
        else:
            d = -(2.0 * math.pi - b + a)
    else:
        # go anti clockwise if shorter
        if a - b <= math.pi:
            d = b - a
        # else go clockwise
        else:
            d = 2 * math.pi - a + b
    assert -math.pi <= d <= math.pi
    return d


def yaw_delta_size(a, b):
    """
    Returns the absolute value of the minimum yaw distance between two yaws
    In other words, what is the least yaw change the rover needs to take in order to reach it's heading yaw
    """
    assert 0.0 <= a <= math.pi * 2.0
    assert 0.0 <= b <= math.pi * 2.0
    d = yaw_difference(a, b)
    return d


def vector_argument(vector):
    """
    Returns the argument of a vector as an angle from -pi to pi
    """
    vector_argument = np.arctan(vector[1] / vector[0])

    if vector[0] < 0:
        vector_argument -= np.pi * np.sign(vector[1])

    return vector_argument


def distance(current, target):
    return math.sqrt(((current[0] - target[0]) ** 2.0) + ((current[1] - target[1]) ** 2.0))


def crow_fly_target_velocity(current, target):
    """
    :current: 2tuple
    :target: 2tuple
    returns: range: 0, or within [min_speed, max_speed]
    """

    dist = distance(current, target)
    assert dist >= 0.0
    if dist > controller_params.slowdown_distance:
        return controller_params.max_speed
    # in the case that we have less than 2 meters left, we should drive at speed proportional to distance left 
    speed = ((controller_params.slowdown_distance - dist) / controller_params.slowdown_distance) \
           * (controller_params.max_speed - controller_params.min_speed) + controller_params.min_speed
    assert speed <= controller_params.max_speed
    return controller_params.max_speed

def radial_vec_to_tangent(r, d, angle_in, angle_change):
    """
    calculates the radial vector to the tangent point on a circle from its centre
    :param: r: the radius of the circle
    :param: d: the distance from the point on the tangent line to the centre of the circle
    :param: angle_in: The argument of the vector to the centre from the initial point
    :angle_change: the signed change in angle of the rover at the turning point in the centre of the circle
            used to determine which side of the circle to go to

    :returns: a vector from the centre of the circle to its intersection with the tangent line
    """

    # the acute angle between vec_to_point and the radial vector to the tangent point of a circle centred on point
    theta = np.arccos(r / d)

    angle_to_tangent_point = angle_in + np.pi + (theta * np.sign(angle_change))

    return r * np.array([np.cos(angle_to_tangent_point), np.sin(angle_to_tangent_point)])

def radial_vec_to_common_circle_tangent(r, d, angle_in, angle_change, previous_angle_change):
    """
    calculates the radial vector to the tangent point on a circle from its centre, given the centre of another
    circle of the same radius which must share the tangent line
    :param: r: the radius of the circles
    :param: d: the distance between the centres of the circles
    :param: angle_in: The argument of the vector between the circles' centres
    :angle_change: the signed change in angle of the rover at the turning point in the centre of the circle
            used to determine which side of the circle to go to

    :returns: a vector from the centre of the circle to its intersection with the tangent line
    """

    if angle_change / previous_angle_change > 0:
        # the common tangent is parallel to the vector between the centres of the two circles
        theta = np.pi

    else:
        # the acute angle between vec_to_point and the radial vector to the tangent point of a circle centred on point
        theta = np.arccos(2 * r / d)

    angle_to_tangent_point = angle_in + np.pi + np.sign(angle_change) * theta

    return r * np.array([np.cos(angle_to_tangent_point), np.sin(angle_to_tangent_point)])
    
def interpolate_circle_circumference(r, p2, centre, n, padded_path, angle_change):
    """
    creates points evenly spaced around a part-circle arc between the last point in padded_path and p2, appends them to the provided list, 
    and returns the modified list
    :param: n: the number of points to add around the circle
    """
    print(padded_path)
    vec1 = padded_path[-1] - centre
    vec2 = p2 - centre

    print(vec1)
    print(vec2)
    print(r)

    theta = np.sign(angle_change) * np.arccos(np.dot(vec1, vec2) / (r ** 2))
    point_argument = vector_argument(vec1)
    
    for _ in range(n):
        point_argument += theta / (n + 1)

        padded_path.append(centre + r * np.array([np.cos(point_argument), np.sin(point_argument)]))

        print("added second circle point")

    return padded_path

