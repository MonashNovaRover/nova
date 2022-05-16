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
alias visualisation='cd ~/nova_ws/src/visualisation'
alias visualization=visualisation
alias vis=visualisation
alias viz=visualisation
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
alias coms='cd ~/nova_ws/other/coms_utils'
alias fleet='cd ~/nova_ws/src/fleet'

# Camera Aliases
alias auto_cameras='ros2 launch realsense2-camera rs_d400_and_t265_launch.py'
alias auto_view='rviz2 -d ~/nova_ws/src/autonomous/config/auto.rviz'

# Networking Aliases
alias jetson='ssh -Y nvidia@192.168.1.204'
alias j2='ssh -Y nova@192.168.1.204'

# GUI Aliases
alias wombat='cd ~/nova_ws/src/gui/wombatx; npm run start'
alias platypus=wombat

# Launching Aliases
alias base='ros2 launch core base.launch.py'
alias rover='ros2 launch core rover.launch.py'
alias arm='ros2 launch core arm.launch.py'
alias arm_spoof='ros2 launch core arm_spoof.launch.py'
alias sci='ros2 launch core science.launch.py'
alias unity='ros2 launch core visualisation.launch.py'

# Hotspots
alias rescan="sudo nmcli device wifi rescan"
alias liam="sudo nmcli device wifi connect Iphone11 password sjfwf355"
alias harrison="sudo nmcli device wifi connect Harrison\ Verrios’s\ iPhone password 12345678"
alias max="sudo nmcli dev wifi connect 'Redmi Note 10 Pro' password Seagull04"

# DGPS
alias dgps="ros2 launch ublox_dgnss rover.launch.py"


alias pi="ssh ubuntu@192.168.1.203"
alias foxglove_server="ros2-foxy-rosbag.ros2 launch rosbridge_server rosbridge_websocket_launch.xml"
