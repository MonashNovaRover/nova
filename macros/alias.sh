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
alias can='. ~/nova_ws/src/core/macros/can.sh'     # Sets up the CAN lines with a virtual or real CAN
alias wifi='. ~/nova_ws/src/core/macros/wifi.sh'   # Allows easy connection to Wifi over command lines

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
alias tutorials='cd ~/nova_ws/src/tutorials'
alias pic='cd ~/nova_ws/other/pics'
alias pics=pic
alias arduino='cd ~/nova_ws/other/arduinos'
alias arduinos=arduino
alias ik='cd ~/nova_ws/other/ik_machine'

# Networking Aliases
alias jetson='ssh -Y nova@192.168.1.204'
