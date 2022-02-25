# Arm control scheme messages
# Different methods to interpret the joystick input

std_msgs/Header header

# Joint-space vs task-space control
bool ik_linear 0
bool ik_angular 0

# Endpoint-frame control
bool endpoint_frame_linear 0
bool endpoint_frame_angular 0

# Base reference frame offset
# Value of 1, 0 or -1 for left, center or right
int8 base_frame_offset 0
