"""
This file provides ROS topic names which we will use throughout the autonomous package
"""

"""
Description: for transformed pose of the tracking camera's optical centre. 
Type: nav_msgs.msg.Odometry

This topic also follows the standard NOVA Coordinate System:
Nova standard coordinate system (left handed coordinates) AND raw data from the tracking camera:
    +x : forward
    +y : right
    +z : up
"""
tracking_pose_topic = "/t265/odom/sample"

rover_pose_topic = "/rover/pose"

depth_point_cloud_topic = "/camera/depth/color/points"

auto_drive_command_topic = "/autonomous/drive_inputs"
auto_goals_topic = "/autonomous/goals"


# this is the ROS "Frame" which we publish everything to. We don't use ROS transforms
main_frame = "map"

"""
This refers to the position of the camera with respect of the Rover's position (the Rover's position being the centre of 
the middle wheels on the ground.
"""
tracking_camera_extrinsics = [.5, 0., 0.]
