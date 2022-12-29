__package__ = "autonomous"
"""
The 2022 Autonomous package performs many geometric transformations on point-clouds, and converts to 
and from point-clouds frequently. This file contains pure functions to do just that!

References: quat2mat is adapted from example.hpp in librealsense (I would not be able to derive these formulae lmao) 

Coordinate Standards:

Nova standard coordinate system (left handed coordinates) AND raw data from the tracking camera:
    +x : forward
    +y : right
    +z : up

Raw data from the depth camera:
    +z : forward
    +x : right
    +y : down
"""


import numpy as np
from geometry_msgs.msg import Quaternion, Transform


class Q:
    """
    Basic structure for storing quaternions
    """
    def __init__(self, x, y, z, w):
        self.x = x
        self.y = y
        self.z = z
        self.w = w


def camera_extrinsics():
    """
    A camera extrinsics matrix is useful for when the cameras are offset by significant distances. 
    We will assume the cameras have the same optical center, despite them being a couple centimeters apart.
    """
    m = \
    [[1, 0, 0],
     [0, 1, 0],
     [0, 0, 1]]
    return np.array(m)


def quat2mat(q: Q):
    """
    This function is adapted from example.h in librealsense
    :param q: q is a Q quaternion with respect to the left handed coordinate system described above
    :return: (3, 3) ndarray
    """
    m = \
    [[1 - 2 * q.y * q.y - 2 * q.z * q.z, 2 * q.x * q.y - 2 * q.z * q.w, 2 * q.x * q.z + 2 * q.y * q.w],
     [2 * q.x * q.y + 2 * q.z * q.w, 1 - 2 * q.x * q.x - 2 * q.z * q.z, 2 * q.y * q.z - 2 * q.x * q.w],
     [2 * q.x * q.z - 2 * q.y * q.w, 2 * q.y * q.z + 2 * q.x * q.w, 1 - 2 * q.x * q.x - 2 * q.y * q.y]]
    return np.array(m)

def transform_to_quat(transform: Transform):
    qx = transform.rotation.x
    qy = transform.rotation.y
    qz = transform.rotation.z
    qw = transform.rotation.w

    return Q(qx, qy, qz, qw)
    

def get_extrinsics(q_mat):
    """
    Given a raw pose message, we want to create one matrix which can transform all the points
    """
    return np.matmul(camera_extrinsics(), q_mat)

def transform_euler(euler_angles, pts):
    """
    transforms euler angles into quaternions, then uses our quaternion rotation matrix to
    rotate the points
    :param: euler angles: [pitch, roll, yaw] - corresponding to 
    :returns: transformed points by the given rotations
    """
    
    qx, qy, qz, qw = euler_to_quat(euler_angles)    

    mat = quat2mat(Q(qx, qy, qz, qw))
    if len(pts) != 0:
        pts = np.matmul(mat, pts.transpose()).transpose()
    return pts


def euler_to_quat(euler_angles):
    pitch, roll, yaw = euler_angles[0], euler_angles[1], euler_angles[2]

    qx = np.sin(roll/2) * np.cos(pitch/2) * np.cos(yaw/2) - np.cos(roll/2) * np.sin(pitch/2) * np.sin(yaw/2)
    qy = np.cos(roll/2) * np.sin(pitch/2) * np.cos(yaw/2) + np.sin(roll/2) * np.cos(pitch/2) * np.sin(yaw/2)
    qz = np.cos(roll/2) * np.cos(pitch/2) * np.sin(yaw/2) - np.sin(roll/2) * np.sin(pitch/2) * np.cos(yaw/2)
    qw = np.cos(roll/2) * np.cos(pitch/2) * np.cos(yaw/2) + np.sin(roll/2) * np.sin(pitch/2) * np.sin(yaw/2)

    return qx, qy, qz, qw
    

def quat_to_euler(transform: Transform):
    """
    take a pose message, get the quaternion and convert it to Euler angles. Maths shamelessly
    stolen from: 
    https://math.stackexchange.com/questions/2975109/how-to-convert-euler-angles-to-quaternions-and-get-the-same-euler-angles-back-fr
    """

    q = transform_to_quat(transform)
    # getting pitch
    t2 = 2 * (q.w*q.y - q.z*q.x)
    t2 = 1 if t2 > 1 else t2
    t2 = -1 if t2 < -1 else t2
    pitch = np.arcsin(t2)
    # getting roll
    t0 = 2 * (q.w*q.x + q.y*q.z)
    t1 = 1 - 2 * (q.x*q.x + q.y*q.y)
    roll = np.arctan2(t0, t1)
    # getting yaw
    t3 = 2 * (q.w*q.z + q.x*q.y)
    t4 = 1 - 2 * (q.y*q.y + q.z*q.z)
    yaw = np.arctan2(t3, t4)
    return pitch, roll, yaw


def transform_points(transform: Transform, pts: np.array) -> np.array:
    """
    pose_msg: nav_msgs.msg.Odometry message
    pts: numpy array with shape (n, 3)
    """
    if len(pts) != 0:
        pts = transform_from_quat(transform.rotation, pts)
    return pts + [transform.translation.x, transform.translation.y, transform.translation.z]


def transform_from_quat(quat: Quaternion, pts: np.array) -> np.array:
    q = Q(quat.x, quat.y, quat.z, quat.w)
    q_mat = quat2mat(q)

    mat = get_extrinsics(q_mat)
    if len(pts) != 0:
        pts = np.matmul(mat, pts.transpose()).transpose()
    return pts


def transform_points_no_yaw(pose_msg, pts):
    """
    Translates points to their x, y and z coordinates assuming that there is no yaw
    """
    pitch, roll, yaw = quat_to_euler(pose_msg)
    if len(pts) == 0:
        return pts
    return transform_euler((pitch, roll, 0), pts)


def transform_yaw(pose_msg, pts):
    """
    Finishes the above transform by rotating according to the yaw.
    """
    pitch, roll, yaw = quat_to_euler(pose_msg)
    if len(pts) != 0:
        pts = transform_euler((0, 0, yaw), pts)
    return pts
