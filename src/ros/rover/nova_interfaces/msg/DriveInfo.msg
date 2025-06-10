# This Drive is used with the new ROS2 Control Based Drive
# The Old Interfaces are still left in the drive package

# speed multiplier
float32 multiplier

#bool for locked
bool locked

#bool for autonomous_mode
bool autonomous_mode

#bool for gamepad connected
bool connected

#enum for drive mode
byte PIVOT = 0
byte HOLONOMIC = 1
byte STRAFE = 2
byte DIFF = 3


#current drive mode of the rover
byte drive_mode 