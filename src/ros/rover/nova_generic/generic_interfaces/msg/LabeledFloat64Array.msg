# A generic data message that includes a timestamped header,
# a list of string labels, and a list of 32-bit floating-point values.
#
# This message is intended for use in systems where multiple numeric values—such as
# sensor readings, computed metrics, or feature data—need to be transmitted alongside
# human-readable or machine-readable labels that describe each value.
#
# The header allows this message to be synchronized with other data sources in time and space,
# while the `labels` and `data` fields carry the core labeled numeric information.

std_msgs/Header header
string[] labels
float64[] data
