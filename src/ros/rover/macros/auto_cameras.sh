#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
#   Runs the autonomous cameras launch file
#   for ros-1 or ros-2
#
# +--------------------------------------------+

# Check if environmental variable is ROS 1
if [[ $ROS_VERSION -eq 1 ]]
then

    # Run the ROS-1 commands and launch file for auto cameras in ros-1
    source /opt/ros/melodic/setup.bash
    source ~/catkin_ws/devel/setup.bash

else

    # Run all the ROS 2 commands to load
    source /opt/ros/eloquent/setup.bash
    source ~/nova_ws/install/setup.bash
    cd ~/nova_ws
    ros2 launch realsense2_camera rs_d400_t265_launch.py

fi

# Source the ROS profile for multi-network connections
export FASTRTPS_DEFAULT_PROFILES_FILE=~/nova_ws/src/core/settings/ROS_DEFAULT_FASTRTPS_PROFILES.xml
