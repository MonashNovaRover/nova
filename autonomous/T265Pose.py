#!/usr/bin/python3
"""
acceleration	X, Y, Z values of acceleration, in meters/sec^2
angular_acceleration	X, Y, Z values of angular acceleration, in radians/sec^2
angular_velocity	X, Y, Z values of angular velocity, in radians/sec
mapper_confidence	Pose map confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High
rotation	Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (relative to initial position)
tracker_confidence	Pose confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High
translation	X, Y, Z values of translation, in meters (relative to initial position)
velocity	X, Y, Z values of velocity, in meters/sec
"""
import math

import pyrealsense2 as rs
import rospy
from common.msg import AutoPose
import sys

# Declare RealSense pipeline, encapsulating the actual device and sensors
pipe = rs.pipeline()

# Build config object and request pose data
cfg = rs.config()
cfg.enable_stream(rs.stream.pose)

initial_x = 0.0
initial_y = 0.0
initial_yaw = 5.0 / 4.0 * math.pi

# Start streaming with requested config
pipe.start(cfg)

rospy.init_node("t265_pose")

publisher = rospy.Publisher("/rover/t265_pose", AutoPose, queue_size=50)

try:
    while not rospy.is_shutdown():
        # Wait for the next set of frames from the camera

        frames = pipe.wait_for_frames()

        # todo: change to rospy rate
        rate = rospy.Rate(10)

        # Fetch pose frame
        pose = frames.get_pose_frame()
        if pose:
            msg = AutoPose()
            data = pose.get_pose_data()
            # calculate position - flip convert to correct x and y conventions
            msg.longitude = -data.translation.z + initial_y
            msg.latitude = data.translation.x + initial_x

            # calculate yaw - convert from quaternion to euler
            qx = data.rotation.x
            qy = data.rotation.z
            qz = data.rotation.y
            qw = data.rotation.w

            # msg.yaw = euler_from_quaternion([q_x, q_y, q_z, q_w])[1]
            yaw = -math.atan2(2.0*(qx*qy + qw*qz), qw*qw + qx*qx - qy*qy - qz*qz)
            yaw = (yaw if yaw > 0 else 2.0 * math.pi + yaw) + 0
            yaw += initial_yaw
            yaw = yaw if yaw <= math.pi * 2 else yaw - math.pi * 2
            
            msg.yaw = yaw

            # calculate velocity - use pythagoras
            x_vel = data.velocity.x
            z_vel = data.velocity.z
            msg.velocity = math.sqrt(x_vel ** 2 + z_vel ** 2)

            # calculate angular velocity - extracting correct axis
            
            msg.angular_velocity = -data.angular_velocity.y

            sys.stdout.write("\r" + "x: " + str(round(msg.latitude, 4)).ljust(7) + " | y: " + str(round(msg.longitude, 4)).ljust(7) + " | yaw: " + str(round(msg.yaw, 4)).ljust(7) + " | vel: " + str(round(msg.velocity, 4)).ljust(7) + " | ang_vel: " + str(round(msg.angular_velocity, 4)).ljust(7))
            sys.stdout.flush()
            publisher.publish(msg)

            rate.sleep()
finally:
    pipe.stop()
