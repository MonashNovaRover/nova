import pyrealsense2 as rs
import numpy as np
#import cv2i
import logging

# reset usbs and get info
ctx = rs.context()
devices = ctx.query_devices()
for dev in devices:
    dev.hardware_reset()
    print(dev.get_info)

# Configure depth and color streams...
# ...from Depth camera
pipeline_1 = rs.pipeline()
config_1 = rs.config()
config_1.enable_device('932122060332')
config_1.enable_stream(rs.stream.depth, rs.format.z16, 30)
config_1.enable_stream(rs.stream.color, rs.format.bgr8, 30)
#config_1.enable_stream(rs.stream.depth, 640, 480, rs.format.z16, 30)
#config_1.enable_stream(rs.stream.color, 640, 480, rs.format.bgr8, 30)
# ...from Tracking camera
pipeline_2 = rs.pipeline()
config_2 = rs.config()
config_2.enable_device('952322110473')
config_2.enable_stream(rs.stream.pose)

# wheel odom
wheel_velocity = rs.vector()
#not finished

# Start streaming from both cameras
pipeline_1.start(config_1)
pipeline_2.start(config_2)

try:
    while True:

        # Camera 1
        # Wait for a coherent pair of frames: depth and color
        frames_1 = pipeline_1.wait_for_frames()
        depth_frame_1 = frames_1.get_depth_frame()
        color_frame_1 = frames_1.get_color_frame()
        if not depth_frame_1 or not color_frame_1:
            continue

        # Camera 2
        # Wait for a coherent pair of frames: depth and color
        frames_2 = pipeline_2.wait_for_frames()
        pose_frame_2 = frames_2.get_pose_frame()
        data = pose_frame_2.get_pose_data()

        print(data.translation)
        if not pose_frame_2:
            continue


finally:

    # Stop streaming
    pipeline_1.stop()
    pipeline_2.stop()
