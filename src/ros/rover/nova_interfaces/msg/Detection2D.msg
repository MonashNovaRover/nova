# A 2D detection result

# The classification of the detected object
string class

# Confidence score [0-1]
float64 score

# Pose of the detected object on the image. X and Y should be set to the corresponding
# pixel coordinates while Z should be 0.
geometry_msgs/Pose pose

# Resolution of the image the detection pose is in reference to
uint16 image_width
uint16 image_height