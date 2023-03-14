# Goal types for the path planner and controller
uint8 GOAL_TYPE_HONING = 0
uint8 GOAL_TYPE_SPIN = 1
uint8 GOAL_TYPE_TAG = 2
uint8 GOAL_TYPE_BLOCK = 3
uint8 GOAL_TYPE_GATE = 4

# Metadata for the tag or block we are searching for
uint32 tag_id
std_msgs/String block_color

uint8 type
Point2D position
