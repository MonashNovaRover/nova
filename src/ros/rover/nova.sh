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

# Source the aliases
source ~/nova_ws/src/core/macros/alias.sh

# Sources the correct ROS bash file
source ~/nova_ws/src/core/macros/ros.sh