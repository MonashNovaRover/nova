import math

import pyrealsense2.pyrealsense2 as rs

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


for i in range(0, 100):
        frames = pipe.wait_for_frames()
        pose = frames.get_pose_frame()
        if pose:
            data = pose.get_pose_data()
            # calculate position - flip convert to correct x and y conventions
            longitude = -data.translation.z + initial_y
            latitude = data.translation.x + initial_x
            print(latitude, longitude)
