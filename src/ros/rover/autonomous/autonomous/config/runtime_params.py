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

# speed of autonomous driving and turning
small_turn_angle = np.pi/16
big_turn_drive_fraction = 0.20
small_turn_drive_fraction = 0.15
straight_drive_fraction = 0.2

# Angle of lenience on completing a full turn on the spot
spin_achieved_delta = np.pi/10

controller_ros_rate = 10  # 10hz

# ~~~~~~~~~~~~~~VIS CONSTANTS ~~~~~~~~~~~~~~~~
pub_scale = 1.0

# ~~~~~~~~~~ PLANNING CONSTANTS ~~~~~~~~~~~~~~~

planning_rate = 2.0
INITIAL_PADDING_DIST_M = 1.0
min_ar_distance = 0.7
max_ar_distance = 20

# ~~~~~~~~~~ MAPPING CONSTANTS ~~~~~~~~~~~~~~~~
min_point_density = 1  # number of points in voxel before we accept it
max_point_depth = 6.0  # distance (m) beyond which we don't consider points
max_fov_horizontal = np.pi / 5  # fov of depth camera for mapping
max_fov_vertical = np.pi / 8  # fov of depth camera for mapping
max_safe_obstacle = 30  # obstacle threshold for 2d height mapping
max_safe_inc = 20  # gradient cutoff for obstacles
unseen_map_val = 0.25  # Fill all points we haven't seen with a set cost to preference known paths
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
