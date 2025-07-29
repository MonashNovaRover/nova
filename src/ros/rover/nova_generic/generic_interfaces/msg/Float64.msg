# A generic data message that includes a timestamped header and a single floating-point value.
#
# This message is intended for use in systems where a numeric reading, measurement, or computed result
# needs to be transmitted along with standard metadata such as timestamp and coordinate frame.
#
# The header allows this message to be synchronized with other data sources in time and space,
# while the `data` field carries the core measurement or value.

std_msgs/Header header
float64 data
