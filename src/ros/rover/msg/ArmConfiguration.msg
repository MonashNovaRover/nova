# Arm configuration messages
# Info on the current physical hardware comprising the arm

std_msgs/Header header

# Joint Info
string[] joint_names
float64[] joint_limits_lower
float64[] joint_limits_upper

# Endpoint Info
string[] endpoint_names
