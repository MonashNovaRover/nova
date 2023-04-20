# Goal types for the path planner and controller
uint8 GOAL_TYPE_INTERMEDIATE = 0
uint8 GOAL_TYPE_COORDINATE = 1
uint8 GOAL_TYPE_TAG = 2
uint8 GOAL_TYPE_GATE = 3

std_msgs/Header header

# Metadata for the tag or block we are searching for
int32[] tag_ids

uint8 type
Point2D position
