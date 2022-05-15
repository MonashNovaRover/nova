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

# appending to pythonpath for autonomous folders
export PYTHONPATH=$PYTHONPATH:~/nova_ws/src/autonomous/autonomous
export PYTHONPATH=$PYTHONPATH:~/nova_ws/src/gui/gui/

# Source the aliases (if ROS 2)
if [[ $ROS_VERSION -eq 2 ]]; then
    source ~/nova_ws/src/core/macros/alias.sh
fi

# set the location of .vimrc
export VIMINIT="source ~/nova_ws/src/core/settings/.vimrc"
