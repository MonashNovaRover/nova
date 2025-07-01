# A generic data message that includes a timestamped header and a single integer value.
#
# This message is intended for use in systems where a discrete numeric value—such as CAN data,
# or status code—needs to be transmitted along with standard metadata such as timestamp and
# coordinate frame.
#
# The header allows this message to be synchronized with other data sources in time and space,
# while the `data` field carries the core integer value.

std_msgs/Header header
int32 data
