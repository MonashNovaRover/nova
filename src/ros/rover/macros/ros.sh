#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# Loads the correct ROS environment based on the
#   ROS saved variable.
#
# +--------------------------------------------+

# Finds if the ROS settings file exists
FILE=~/nova_ws/src/core/settings/ROS_SETTINGS.sh
if [[ -f "$FILE" ]]
then

    # Reads the ROS Settings script
    source $FILE

# Otherwise create it
else

    # Create and populates the file
    mkdir -p ~/nova_ws/src/core/settings
    touch $FILE
    echo "export ROS_VERSION=2" > $FILE
    export ROS_VERSION=2

fi

# Check if environmental variable is ROS 1
if [[ $ROS_VERSION -eq 1 ]]
then

    # Run all the ROS 1 commands to load
    source /opt/ros/melodic/setup.bash
    source ~/catkin_ws/devel/setup.bash
    source ~/catkin_ws/src/common/aliases.sh

else

    # Run all the ROS 2 commands to load
    source /opt/ros/eloquent/setup.bash
    source ~/nova_ws/install/setup.bash

fi

# Source the ROS profile for multi-network connections
unset FASTRTPS_DEFAULT_PROFILES_FILE

# Needs to be fixed
#export FASTRTPS_DEFAULT_PROFILES_FILE=~/nova_ws/src/core/settings/ROS_DEFAULT_FASTRTPS_PROFILES.xml
