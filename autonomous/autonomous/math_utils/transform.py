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
import copy
from geometry_msgs.msg import Quaternion, Transform, Pose
from quaternions import Quaternion as MathQuaternion


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


def quat2mat(q: Quaternion):
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
    
    if len(pts) != 0:
        quat = euler_to_quat(euler_angles)    

        mat = quat2mat(quat)
        pts = np.matmul(mat, pts.transpose()).transpose()
    return pts


def euler_to_quat(euler_angles):
    pitch, roll, yaw = euler_angles[0], euler_angles[1], euler_angles[2]

    quat = Quaternion()
    quat.x = np.sin(roll/2) * np.cos(pitch/2) * np.cos(yaw/2) - np.cos(roll/2) * np.sin(pitch/2) * np.sin(yaw/2)
    quat.y = np.cos(roll/2) * np.sin(pitch/2) * np.cos(yaw/2) + np.sin(roll/2) * np.cos(pitch/2) * np.sin(yaw/2)
    quat.z = np.cos(roll/2) * np.cos(pitch/2) * np.sin(yaw/2) - np.sin(roll/2) * np.sin(pitch/2) * np.cos(yaw/2)
    quat.w = np.cos(roll/2) * np.cos(pitch/2) * np.cos(yaw/2) + np.sin(roll/2) * np.sin(pitch/2) * np.sin(yaw/2)

    return quat
    

def quat_to_euler(q: Quaternion):
    """
    take a pose message, get the quaternion and convert it to Euler angles. Maths shamelessly
    stolen from: 
    https://math.stackexchange.com/questions/2975109/how-to-convert-euler-angles-to-quaternions-and-get-the-same-euler-angles-back-fr
    """
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


def transform_from_quat(q: Quaternion, pts: np.array) -> np.array:
    q_mat = quat2mat(q)

    mat = get_extrinsics(q_mat)
    if len(pts) != 0:
        pts = np.matmul(mat, pts.transpose()).transpose()

    return pts


def transform_points_no_yaw(transform: Transform, pts):
    """
    Translates points to their x, y and z coordinates assuming that there is no yaw
    """
    pitch, roll, yaw = quat_to_euler(transform.rotation)
    if len(pts) == 0:
        return pts
    return transform_euler((pitch, roll, 0), pts)


def transform_yaw(transform: Transform, pts):
    """
    Finishes the above transform by rotating according to the yaw.
    """
    pitch, roll, yaw = quat_to_euler(transform.rotation)
    if len(pts) != 0:
        pts = transform_euler((0, 0, yaw), pts)
    return pts

def quaternion_multiply(quat0: Quaternion, quat1: Quaternion) -> Quaternion:
    """
    Returns the product of a quaternion multiplication
    """
    q0 = MathQuaternion(quat0.w, quat0.x, quat0.y, quat0.z)
    q1 = MathQuaternion(quat1.w, quat1.x, quat1.y, quat1.z)
    q = q1 * q0
    ret_q = Quaternion()
    ret_q.w, ret_q.x, ret_q.y, ret_q.z = q.real, q.i, q.j, q.k
    return ret_q

def quaternion_right_divide(quat0: Quaternion, quat1: Quaternion) -> Quaternion:
    """
    Returns the quotient of a quaternion division, inverting the second quaternion then multiplying
    """
    quat1 = copy.deepcopy(quat1)
    quat1.w = -quat1.w
    return quaternion_multiply(quat0, quat1)


def quaternion_left_divide(quat0: Quaternion, quat1: Quaternion) -> Quaternion:
    """
    Returns the quotient of a quaternion division, inverting the second quaternion then multiplying
    """
    quat0 = copy.deepcopy(quat0)
    quat0.w = -quat0.w
    return quaternion_multiply(quat0, quat1)


def transform_pose(pose: Pose, transform: Transform) -> Pose:
    """
    Transform a pose message according to a transform
    """
    return_pose = Pose()
    pts = np.array([[pose.position.x, pose.position.y, pose.position.z]])
    new_pts = transform_points(transform=transform, pts=pts).flatten()
  
    return_pose.position.x, return_pose.position.y, return_pose.position.z = new_pts[0], new_pts[1], new_pts[2]
    return_pose.orientation = quaternion_multiply(transform.rotation, pose.orientation)

    return return_pose

def offset_transform(transform: Transform, offset: Transform):
    """
    Returns the transformation of a fixed pose attached to a transformation
    :param transform: The transform applied to the coordinate frame
    :param offset: The offset of the initial frame from the pose being transformed
    :returns: The offset transform undergone by the point to arrive at its final position
    NOTE: Currently only works if the offset has no rotation
    TODO: combine offset and transform quaternions to allow rotated offsets
    """
    transformed = Transform()
    non_offset_transform = Transform()

    print(f"offset: {offset}")
    print(f"transform rotation: {transform.rotation}")
    # quaternion multiplication to transform rotation into frame
    transform_rotation_in_offset_frame = quaternion_left_divide(offset.rotation, transform.rotation)
    print(f"half transformed rotation = {transform_rotation_in_offset_frame}")
    transformed.rotation = quaternion_multiply(transform_rotation_in_offset_frame, offset.rotation)
    print(f"transformed rotation: {transformed.rotation}")
    non_offset_transform.rotation = transformed.rotation

    # rotate translation into correct frame
    translation_point = np.array([transform.translation.x, transform.translation.y, transform.translation.z])
    non_offset_transform.translation.x, non_offset_transform.translation.y, non_offset_transform.translation.z = \
        transform_from_quat(offset.rotation, translation_point)

    # transform to offset frame
    external_point = -np.array([offset.translation.x, offset.translation.y, offset.translation.z])

    # do transform
    transformed_point = transform_points(non_offset_transform, external_point).flatten()

    # undo transformed offset to get back to original frame
    transformed.translation.x, transformed.translation.y, transformed.translation.z = transformed_point - external_point

    return transformed

    
