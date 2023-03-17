# The speed value specifies the wheel speed (as a percentage) 
#   to drive at, between -1 and 1. This is equivalent to % RPM.
float32 speed

# The steer value specifies whether the drive should move left
#   (-1) or right (1).
float32 steer

# The angle is the angle to move the rover in. only applicable in
# strafe mode
float32 angle

# Determines whether the rover is in strafe mode
#   true = strafe, false = normal
bool strafe_mode
