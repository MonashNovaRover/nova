# This DriveInput is used with the new ROS2 Control Based Drive
# The Old Interfaces are still left in the drive package

# Use DriveInputStamped or TwistStamped for Drive

# The speed value specifies the wheel speed (as a percentage)
#   to drive at, between -1 and 1. This is equivalent to % RPM.
float64 speed

# The radius value specifies the turning radius of the rover in m, positive is left
# negative is right.
float64 radius

# The direction of the turning radius, left 1, right -1
int8 direction