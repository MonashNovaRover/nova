# Stores a list of goals we want to reach (in order) to achieve a goal in autonomous
# If there are n goals, the first n-1 goals must be of type GOAL_TYPE_INTERMEDIATE, 
# and the final goal should indicate the type of goal we are looking for (GOAL_TYPE_COORDINATE, 
#   GOAL_TYPE_GATE or GOAL_TYPE_TAG)

std_msgs/Header header

core/AutonomousGoal[] goals