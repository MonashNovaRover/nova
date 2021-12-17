import math

# tank turning constants (test and define on a terrain basis)
min_yaw_rate = 4.0  # what is a minimum fair speed to turn slowly
max_yaw_rate = 5.0  # # what is a fair maximum yaw percent speed to turn

min_yaw_difference = math.pi / 25.0  # arbitrary for now

slowdown_distance = 2.0

corner_padding = 0.3    # Radius by which to avoid corners, in m

circle_interpolation_num_points = 4

corner_angle_threshold = 0.1    # minimum angle change (radians) for a point to be considered a corner

min_speed = 2.0   # todo: determiner

max_speed = 7.0  # todo: determine

min_waypoint_distance = 0.2  # todo: determine what is achievable

controller_ros_rate = 0.1  # 10hz

a_star_rate = 1 # 1Hz
