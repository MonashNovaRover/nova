__package__ = "autonomous"
import numpy as np

# ~~~~~~~~~~ CONTROLLER CONSTANTS ~~~~~~~~~~~~~
# tank turning constants (test and define on a terrain basis)
min_yaw_rate = 4.0  # what is a minimum fair speed to turn slowly
max_yaw_rate = 5.0  # # what is a fair maximum yaw percent speed to turn

min_yaw_difference = np.pi / 25.0  # arbitrary for now

slowdown_distance = 2.0

min_speed = 2.0   # todo: determiner

max_speed = 7.0  # todo: determine

min_waypoint_distance = 0.5  # todo: determine what is achievable

controller_ros_rate = 10  # 10hz

# ~~~~~~~~~~ PLANNING CONSTANTS ~~~~~~~~~~~~~~~
a_star_rate = 1 # 1Hz

# ~~~~~~~~~~ MAPPING CONSTANTS ~~~~~~~~~~~~~~~~
min_point_density = 2 # number of points in voxel before we accept it
max_point_depth = 3.5 # distance beyond which we don't consider points
max_fov_angle = np.pi/6 #40 degrees
max_safe_obstacle = 36 # obstacle threshold for 2d map
max_safe_inc = 30 # gradient cutoff for obstacles
depth_mode = "python" # whether we publish points over ros or use a python callback
skip_pts = 1 # Step we use when selecting points if we can't handle the full cloud
unseen_map_val = 0.1 # Fill all points we haven't seen with a set cost to preference known paths

# ~~~~~~~~~~~~~~~~~ CAMERA INFO ~~~~~~~~~~~~~~~~~~~~~
d415_serial = "932122060332"
d435_serial = "829212072166"
active_depth_camera = d435_serial
t265_serial = "952322110473"
# position of centre of wheel base relative to tracking cam
tracking_camera_extrinsics = [-.48, 0., -0.48]
