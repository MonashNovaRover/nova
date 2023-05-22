import numpy as np

# ~~~~~~~~~~ CONTROLLER CONSTANTS ~~~~~~~~~~~~~

min_yaw_difference = np.pi / 25.0  # this number has worked well for us so far

# speed of autonomous driving and turning
small_turn_angle = np.pi/16
big_turn_drive_fraction = 0.20
small_turn_drive_fraction = 0.15
straight_drive_fraction = 0.2

# Angle of lenience on completing a full turn on the spot
spin_achieved_delta = np.pi/5

# ~~~~~~~~~~ PLANNING CONSTANTS ~~~~~~~~~~~~~~~
INITIAL_PADDING_DIST_M = 0.9
min_ar_distance = 0.7
sim_ar_tag_range = 8

# ~~~~~~~~~~ MAPPING CONSTANTS ~~~~~~~~~~~~~~~~
min_point_density = 1  # number of points in voxel before we accept it
max_point_depth = 6.0  # distance (m) beyond which we don't consider points
max_fov_horizontal = np.pi / 5  # fov of depth camera for mapping
max_fov_vertical = np.pi / 8  # fov of depth camera for mapping
max_safe_obstacle = 50  # obstacle threshold for 2d height mapping
max_safe_inc = 40  # gradient cutoff for obstacles
unseen_map_cost = 0.25  # Fill all points we haven't seen with a set cost to preference known paths
unseen_map_val = -1  # The value we set to indicate unknown areas - this is useful for visualisation
obstacle_halve_value = 20  # All costs below a scaled value of 80 are halved to be more decisive
obstacle_ignore_value = 5  # All costs below a scaled value of 30 are ignored

# ~~~~~~~~~~~~~~~~~ CAMERA INFO ~~~~~~~~~~~~~~~~~~~~~
d415_serial = "932122060332"
d435_serial = "829212072166"
d455_serial = "213522254970"
active_depth_camera = d455_serial  # d435_serial
t265_serial = "952322110473"
pose_file = "cameras/pose.txt"
