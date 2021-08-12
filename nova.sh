#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# Core Repository Bash Script
# This should be added to everyone's .bashrc file
#       sudo echo "source ~/nova_ws/src/core" < ~/.bashrc
# This will initialise all macros and set up ROS correctly
#
# +--------------------------------------------+

# Macro Aliases
alias build='. ~/nova_ws/src/core/macros/build.sh' # Runs colcon build
alias setup='. ~/nova_ws/install/setup.bash'       # Sets up the ROS repository when scripts change
alias pull='. ~/nova_ws/src/core/macros/pull.sh'   # Runs a GitHub pull on all repositories

# Directory Aliases
alias nova='cd ~/nova_ws'
alias core='cd ~/nova_ws/src/core'
alias control='cd ~/nova_ws/src/control'
alias electronics='cd ~/nova_ws/src/electronics'
alias elec=electronics
alias science='cd ~/nova_ws/src/science'
alias cameras='cd ~/nova_ws/src/cameras'
alias cams=cameras
alias autonomous='cd ~/nova_ws/src/autonomous'
alias auto=autonomous
alias gui='cd ~/nova_ws/src/gui'

# Calls bash executions
setup                                              # Sets up the bash file
source /opt/ros/eloquent/setup.bash                # Sources ROS 2 Eloquent
. ~/nova_ws/install/setup.bash                     # Runs the workspace setup file
