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

# Calls bash executions
setup                                              # Sets up the bash file
source /opt/ros/eloquent/setup.bash                # Sources ROS 2 Eloquent
. ~/nova_ws/install/setup.bash                     # Runs the workspace setup file
