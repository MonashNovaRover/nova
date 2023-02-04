# An angular velocity measurement and other relevant CMD values
# paired with a time they werer read.
# Used for reporting true velocities, currents, temperaturs and powers
# back to the PID tuner, for display on the GUI, and for dead
# reckoning

# Header
std_msgs/Header header

# The Bus Index of the CMD
int8 bus

# The CMD Index that is being used
int8 id

# Angular velocity of motor in rad/s (optional)
float64 omega

# Linear velocity in m/s (optional, for wheels or linear actuators)
float64 vel

# PWM duty cycle [0, 1]
float64 duty_cycle

# Current draw in Amps
float64 current

# Encoder interval
int16 interval