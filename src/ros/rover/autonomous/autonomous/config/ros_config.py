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
    +y : left
    +z : up
"""
# ~~~~~~~~~~~~~ Tracking & Depth cams ~~~~~~~~~~~~~~~~ 
depth_topic = "/D400/depth/color/points"
depth_point_cloud_topic = "/camera/depth/color/points"
# position of centre of wheel base relative to tracking cam
tracking_camera_extrinsics = [-.48, 0., -0.48] 

# ~~~~~~~~~~~~~~~~~~ Autonomous ~~~~~~~~~~~~~~~~~~~~~~
auto_drive_command_topic = "/control/autonomous_commands"
rover_pose_topic = "/rover/pose"
ar_track_topic = "/autonomous/ar_tag"
auto_waypoints_topic = "/autonomous/waypoints"
planning_destination_topic = "/autonomous/planning_destination"
auto_goal_gps = "/autonomous/goal/gps"
auto_goal_topic = "/autonomous/goal"
gps_to_xyz_topic = "/autonomous/gps_to_xyz"
xyz_to_gps_topic = "/autonomous/xyz_to_gps"
occupancy_grid_topic = "/autonomous/occupancy_grid"
path_planning_service_name = "autonomous/path_planning_service"
