#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# Core Repository Bash Script
# This should be added to everyone's .bashrc file
#       sudo echo "source ~/nova_ws/src/core" < ~/.bashrc
# This will initialise all macros and set up ROS correctly.
#
# +--------------------------------------------+

# Sources the correct ROS bash file
source ~/nova_ws/src/core/macros/ros.sh

# Source the aliases (if ROS 2)
if [[ $ROS_VERSION -eq 2 ]]; then
    source ~/nova_ws/src/core/macros/alias.sh
fi