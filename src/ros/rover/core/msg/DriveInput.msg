#message for the inputs to the driver

#enum for the driving mode
byte PIVOT = 1
byte STRAFE = 2
byte TANK = 3

# The speed value specifies the wheel speed (as a percentage)
#   to drive at, between -1 and 1. This is equivalent to % RPM.
float32 speed

# The radius value specifies the turning radius of the rover, positive is left
# negative is right.
float32 radius

# Determines the drive mode the rover is in.
byte mode

# Determines if the handbrake is on
bool handbrake