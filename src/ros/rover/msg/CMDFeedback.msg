# An RPM velocity measurement paired with a time it was read.
# Used for reporting true velocities back to the PID tuner

# The Bus Index of the CMD
int8 bus

# The CMD Index that is being used
int8 id

# True rpm from sensor
float64 rpm
float64 power

# Time in milliseconds since epoch at which reading was taken
int64 time
