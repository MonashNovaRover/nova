#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# This script stores all of the aliases to
#   other scripts and other bash files.
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
alias pic='cd ~/nova_ws/other/pics'
alias pics=pic
alias arduino='cd ~/nova_ws/other/arduinos'
alias arduinos=arduino
