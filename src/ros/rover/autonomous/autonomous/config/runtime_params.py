__package__ = "autonomous"

import numpy as np

# ~~~~~~~~~~ CONTROLLER CONSTANTS ~~~~~~~~~~~~~
# tank turning constants (test and define on a terrain basis)
yaw_star_conf = True
# the following two parameters will some day be used for good
min_yaw_rate = 4.0  # what is a minimum fair speed to turn slowly
max_yaw_rate = 5.0  # # what is a fair maximum yaw percent speed to turn

min_yaw_difference = np.pi / 25.0  # this number has worked well for us so far

dist_through_gate_m = 2 # the distance we drive through the gate before stopping

slowdown_distance = 2.0 # don't use these ones either lol

min_speed = 2.0  # todo: determine

max_speed = 7.0  # todo: determine

ignore_waypoints = 4  # number of waypoints to cut off start of list

# speed of autonomous driving and turning
small_turn_angle = np.pi/16
big_turn_drive_fraction = 0.30
small_turn_drive_fraction = 0.15
straight_drive_fraction = 0.6

# Angle of lenience on completing a full turn on the spot
spin_achieved_delta = np.pi/10

controller_ros_rate = 10  # 10hz

# ~~~~~~~~~~~~~~VIS CONSTANTS ~~~~~~~~~~~~~~~~
pub_scale = 1.0

# ~~~~~~~~~~ PLANNING CONSTANTS ~~~~~~~~~~~~~~~

planning_rate = 2.0
INITIAL_PADDING_DIST_M = 0.8
min_ar_distance = 0.7
max_ar_distance = 20

# ~~~~~~~~~~ MAPPING CONSTANTS ~~~~~~~~~~~~~~~~
min_point_density = 3  # number of points in voxel before we accept it
max_point_depth = 6  # distance beyond which we don't consider points
max_fov_angle = np.pi / 6  # fov of depth camera for mapping
max_safe_obstacle = 30  # obstacle threshold for 2d height mapping
max_safe_inc = 20  # gradient cutoff for obstacles
depth_mode = "python"  # whether we publish points over ros or use a python callback
unseen_map_val = 0.25  # Fill all points we haven't seen with a set cost to preference known paths
slice_height = 2.3  # the height we slice the map from when taking 2d slices
obstacle_halve_value = 50  # All costs below a scaled value of 80 are halved to be more decisive
obstacle_ignore_value = 20  # All costs below a scaled value of 30 are ignored
min_map_update_time = .2  # minimum time between updating the map from point-cloud

# ~~~~~~~~~~~~~~~LOCALISATION CONSTANTS ~~~~~~~~~~~~~~~~~
minimum_gps_corrections = 50
pose_pub_rate = 0.2

# ~~~~~~~~~~~~~~~~~ CAMERA INFO ~~~~~~~~~~~~~~~~~~~~~
d415_serial = "932122060332"
d435_serial = "829212072166"
d455_serial = "213522254970"
active_depth_camera = d455_serial  # d435_serial
t265_serial = "952322110473"
pose_file = "cameras/pose.txt"
