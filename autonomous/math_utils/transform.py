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


def quat2mat(q):
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


def get_pc_rotation_matrix(pose_msg):
    """
    Given a raw pose message, we want to create one matrix which can transform all the points
    """

    qx = pose_msg.pose.pose.orientation.x
    qy = pose_msg.pose.pose.orientation.y
    qz = pose_msg.pose.pose.orientation.z
    qw = pose_msg.pose.pose.orientation.w

    q = Q(qx, qy, qz, qw)
    
    return np.matmul(camera_extrinsics(), quat2mat(q))


def transform_points(pose_msg, pts):
    """
    pose_msg: nav_msgs.msg.Odometry message
    pts: numpy array with shape (n, 3)
    """
    mat = get_pc_rotation_matrix(pose_msg)
    pts = np.matmul(mat, pts.transpose()).transpose()
    pts = pts + [pose_msg.pose.pose.position.x, pose_msg.pose.pose.position.y, pose_msg.pose.pose.position.z]
    return pts
