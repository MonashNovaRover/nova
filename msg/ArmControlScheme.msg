# Arm control scheme messages
# Different methods to interpret the joystick input

std_msgs/Header header

# Joint-space vs task-space control
bool ik_lower_joints 0
bool ik_wrist 0

# Camera-frame control
bool camera_frame_linear 0
bool camera_frame_angular 0

# Base reference frame offset
# Value of 1, 0 or -1 for left, center or right
int8 base_frame_offset 0
