# Container to hold CMD feedback for all CMDs 

# Header object
std_msgs/Header header

# CMDFeedback array for wheel motors
CMDFeedback[] wheels

# CMDFeedback array for wheel pivot motors (relevant for new suspension)
CMDFeedback[] wheel_pivots

# CMDFeedback for arm motors (defined from base to end-effector)
CMDFeedback[] arm_motors

# CMDFeedback for science motors (if used)
CMDFeedback[] science_motors