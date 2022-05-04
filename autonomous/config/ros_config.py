__package__ = "autonomous"
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
# ~~~~~~~~~~~~~ Tracking & Depth cams ~~~~~~~~~~~~~~~~ 
tracking_pose_topic = "/t265/odom/sample"
rover_odom_topic = "/rover/odom"
depth_topic = "/D400/depth/color/points"
depth_point_cloud_topic = "/camera/depth/color/points"
# position of centre of wheel base relative to tracking cam
tracking_camera_extrinsics = [-.48, 0., -0.48] 

# ~~~~~~~~~~~~~~~~~~ Autonomous ~~~~~~~~~~~~~~~~~~~~~~
auto_drive_command_topic = "/autonomous/drive_inputs"
rover_pose_topic = "/rover/pose"
ar_track_topic = "/autonomous/ar_tag"
auto_waypoints_topic = "/autonomous/waypoints"
auto_goals_topic = "/autonomous/goals"
auto_goals_info = "/autonomous/goals/info"
ar_goals_topic = "autonomous/ar_tag/global_odom"

# ~~~~~~~~~~~~~~~~~~~~~ ROS ~~~~~~~~~~~~~~~~~~~~~~~~~~
# this is the ROS "Frame" which we publish everything to. We don't use ROS transforms
main_frame = "map"

