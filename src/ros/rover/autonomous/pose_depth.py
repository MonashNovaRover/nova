import pyrealsense2 as rs

T265_serial = '952322110473' # tracking
D415_serial = '932122060332' # depth

def USB_Reset():
    # Reset USBs and get info
    ctx = rs.context()
    devices = ctx.query_devices()
    for device in devices:
        device.hardware_reset()
        print(device.get_info)

USB_Reset()

# Configure camera streams

# From Depth Camera
pipe1 = rs.pipeline()
cfg1 = rs.config()
cfg1.enable_device(D415_serial)
cfg1.enable_stream(rs.stream.depth, rs.format.z16, 30)
cfg1.enable_stream(rs.stream.color, rs.format.bgr8, 30)

# From Tracking Camera
pipe2 = rs.pipeline()
cfg2 = rs.config()
cfg2.enable_device(T265_serial)
cfg2.enable_stream(rs.stream.pose)

# Wheel Odom - Todo
wheel_velocity = rs.vector()

# Start streaming from both cameras
pipe1.start(cfg1)
pipe2.start(cfg2)

# Retrieve data from cameras
# This would be better in two different files so pose doesn't have to
# wait for depth frames and both could run simultaneously
try:
    while True:
        # Get depth info
        f1 = pipe1.wait_for_frames()
        depth_frame = f1.get_depth_frame()
        colour_frame = f1.get_color_frame()
        # Do normal manipulation and publish ros topic 

        # Get pose info
        f2 = pipe2.wait_for_frames()
        pose_frame = f2.get_pose_frame()
        # Do normal manipulation with wheel odom and publish ros topic

finally:
    # Stop streaming
    pipe1.stop()
    pipe2.stop()