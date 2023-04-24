# The speed value specifies the wheel speed (as a percentage) 
#   to drive at, between -1 and 1. This is equivalent to % RPM.
float32 speed

# The steer value specifies whether the drive should move left
#   (-1) or right (1).
float32 steer

# Determines whether the rover is in strafe mode
#   true = strafe, false = normal
byte PIVOT = 1
byte STRAFE = 2
byte TANK = 3

byte mode
