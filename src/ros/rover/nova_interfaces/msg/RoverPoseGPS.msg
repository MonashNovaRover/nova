# Stores information about the GPS and IMU data
std_msgs/Header header

# If the data is valid
bool valid

# Were we able to get an accurate heading fix?
bool heading_valid

# GPS data
float64 latitude
float64 longitude

# IMU angles
float32 pitch
float32 roll
float32 yaw
