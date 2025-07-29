# A generic data message that includes a timestamped header and a single string value.
#
# This message is intended for use in systems where a text-based value—such as a label, status message,
# command, or identifier needs to be transmitted along with standard metadata such as timestamp and
# coordinate frame.
#
# The header allows this message to be synchronized with other data sources in time and space,
# while the `data` field carries the core string value.

std_msgs/Header header
byte data
