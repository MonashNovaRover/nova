# An angular velocity measurement and other relevant CMD values
# paired with a time they werer read.
# Used for reporting true velocities, currents, temperaturs and powers
# back to the PID tuner, for display on the GUI, and for dead
# reckoning

# The Bus Index of the CMD
int8 bus

# The CMD Index that is being used
int8 id

# Angular velocity of motor in rad/s (optional)
float64 v_angular

# Linear velocity in m/s (optional, for wheels or linear actuators)
float64 v_linear

# Power draw in Watts
float64 power

# Current draw in Amps
float64 current

# Temperature (C)
float64 temperature

# Time in milliseconds since epoch at which reading was taken
int64 time
