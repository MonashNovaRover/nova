# A generic data message that includes a timestamped header and a single boolean value.
#
# This message is intended for use in systems where a binary condition—such as an on/off state,
# success/failure flag, or presence detection needs to be transmitted along with standard metadata
# such as timestamp and coordinate frame.
#
# The header allows this message to be synchronized with other data sources in time and space,
# while the `data` field carries the core boolean value.

std_msgs/Header header
bool data
