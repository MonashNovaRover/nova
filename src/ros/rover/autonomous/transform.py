# quat2mat is adapted from example.hpp in librealsense (I would not be able to derive these formulae lmao) 

import numpy as np

"""
On the tracking camera:
    x : forward
    y : right
    z : up

On the depth camera:
    z : forward
    x : right
    y : down

so we should have swapped the depth value indexes like so, and made the last value negative
[2, 0, 1]
"""

class Q:
    def __init__(self, x, y, z, w):
        self.x = x
        self.y = y
        self.z = z
        self.w = w

def camera_extrinsics():
    # according to librealsense, we need to have the 2nd and 3rd ones negative (why not first and 2nd? I thought z was depth...)
    m = \
    [[1, 0, 0],
     [0, 1, 0],
     [0, 0, 1]]
    return np.array(m)

def quat2mat(q):
    """
    Assume q is a quaternion with y
    """
    m = \
    [[1 - 2 * q.y * q.y - 2 * q.z * q.z, 2 * q.x * q.y - 2 * q.z * q.w, 2 * q.x * q.z + 2 * q.y * q.w],
     [2 * q.x * q.y + 2 * q.z * q.w, 1 - 2 * q.x * q.x - 2 * q.z * q.z, 2 * q.y * q.z - 2 * q.x * q.w],
     [2 * q.x * q.z - 2 * q.y * q.w, 2 * q.y * q.z + 2 * q.x * q.w, 1 - 2 * q.x * q.x - 2 * q.y * q.y]]
    return np.array(m)

def get_pc_transformation(pose_msg):
    """
    Given a raw pose message, we want to create one matrix which can transform all the points
    """
    x = pose_msg.pose.pose.position.x 
    y = pose_msg.pose.pose.position.y
    z = pose_msg.pose.pose.position.z 
    
    qx = pose_msg.pose.pose.orientation.x
    qy = pose_msg.pose.pose.orientation.y
    qz = pose_msg.pose.pose.orientation.z
    qw = pose_msg.pose.pose.orientation.w

    q = Q(qx, qy, qz, qw)
    
    return np.matmul(camera_extrinsics(), quat2mat(q))

