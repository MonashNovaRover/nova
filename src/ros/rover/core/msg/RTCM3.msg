# used to transport RAW RTCM3.3 Messages over a ROS topic
std_msgs/Header header # Header timestamp should be time of reception from Base 
uint8[] data            # data bytes, may be up to 1200 bytes in length.