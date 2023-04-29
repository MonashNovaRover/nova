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
alias build='. ~/nova_ws/src/rover/core/macros/build.sh'     # Runs colcon build
alias setup='. ~/nova_ws/install/setup.bash'                 # Sets up the ROS repository when scripts change
alias pull='. ~/nova_ws/src/rover/core/macros/pull.sh'       # Runs a GitHub pull on all repositories
alias can='. ~/nova_ws/src/rover/core/macros/can.sh'         # Sets up the CAN lines with a virtual or real CAN
alias check='. ~/nova_ws/src/rover/core/macros/check.sh'     # Adds an alias for the `check` command for `screen -ls`
alias stop='. ~/nova_ws/src/rover/core/macros/stop.sh'       # Adds an alias for the `stop` command for stopping systemctl services
alias restart='. ~/nova_ws/src/rover/core/macros/restart.sh' # Adds an alias for the `restart` command for restarting systemctl services
alias wifi='. ~/nova_ws/src/rover/core/macros/wifi.sh'       # Allows easy connection to Wifi over command lines
alias docgen='. ~/nova_ws/src/rover/core/macros/docgen.sh'   # Generates documentation located in ~/nova_ws/src/rover/docs/build/html/
alias docinstall='. ~/nova_ws/src/rover/core/macros/apidoc_install.sh'   # Install sphinx-apidoc and friends required for building docs

# Directory Aliases
alias nova='cd ~/nova_ws'
alias rover='cd ~/nova_ws/src/rover'
alias core='cd ~/nova_ws/src/rover/core'
alias control='cd ~/nova_ws/src/rover/control'
alias electronics='cd ~/nova_ws/src/rover/electronics'
alias elec=electronics
alias visualisation='cd ~/nova_ws/src/visualisation'
alias visualization=visualisation
alias vis=visualisation
alias viz=visualisation
alias science='cd ~/nova_ws/src/rover/science'
alias cameras='cd ~/nova_ws/src/cameras'
alias cams=cameras
alias autonomous='cd ~/nova_ws/src/rover/autonomous'
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
alias auto_view='rviz2 -d ~/nova_ws/src/rover/autonomous/config/auto.rviz'
alias arm_view='rviz2 -d ~/nova_ws/src/rover/control/rviz/arm_viz_2023.rviz'

# Networking Aliases
alias jetson='ssh -Y nvidia@192.168.1.204'
alias jetson_wifi='ssh -Y nvidia@tegra-ubuntu'
alias j2='ssh -Y nvidia@192.168.1.204'
alias j2_wifi='ssh -Y nvidia@192.168.0.104'
alias comp='. ~/nova_ws/src/rover/core/macros/comp.sh'

# GUI Aliases
alias wombat='cd ~/nova_ws/src/gui/wombatx; npm run start'
alias platypus=wombat

# Launching Aliases
alias base='ros2 launch core base.launch.py'
alias drive='ros2 launch core drive.launch.py'
alias arm='ros2 launch core arm.launch.py'
alias arm_spoof='ros2 launch core arm_spoof.launch.py'
alias sci='ros2 launch core science.launch.py'
alias unity='ros2 launch core visualisation.launch.py'
alias auto_drive='ros2 launch core auto_drive.launch.py'
alias localisation='ros2 launch core urdf.launch.py'
alias launch_viz='ros2 launch core viz.launch.py' 
alias launch_vis='ros2 launch core viz.launch.py'
# Service Aliases
alias arm_config_info='ros2 service call control/arm_config_info core/srv/ArmConfigInfo'
alias arm_reset_control_pose='ros2 service call control/arm_reset_control_pose std_srvs/srv/Trigger'
alias zero_resolver='~/nova_ws/src/rover/core/macros/zero_resolver.sh'

# eduroam connection
alias eduroam="sudo ip r delete default via 192.168.1.1"

# Hotspots
alias rescan="sudo nmcli device wifi rescan"
alias liam="sudo nmcli device wifi connect Iphone11 password sjfwf355"
alias harrison="sudo nmcli device wifi connect Harrison\ Verrios’s\ iPhone password 12345678"
alias max="sudo nmcli dev wifi connect 'Redmi Note 10 Pro' password Seagull04"

# DGPS
alias dgps="ros2 launch ublox_dgnss rover.launch.py"
alias pi="ssh ubuntu@192.168.1.203"
alias foxglove_server="ros2-foxy-rosbag.ros2 launch rosbridge_server rosbridge_websocket_launch.xml"

# Science commands
alias sci_copy="scp -r nvidia@192.168.1.204:nova_ws/src/rover/science/data/spectrometer ~/nova_ws/src/rover/science/data"

# Creating template program
alias python_template="cp ~/nova_ws/src/rover/core/macros/python_ros_template.py ."

# systemctl / screen aliases
alias check='. ~/nova_ws/src/rover/core/macros/check.sh'     # Sets up the CAN lines with a virtual or real CAN
alias rerun='. ~/nova_ws/src/rover/core/macros/rerun.sh'     # Sets up the CAN lines with a virtual or real CAN

# autonomous aliases
alias play_bag='. ~/nova_ws/src/rover/core/macros/bag_play.sh'
alias rosbridge='ros2 launch rosbridge_server rosbridge_websocket_launch.xml'
alias bag="ros2 bag record /T265/pose /depth_camera/d435_1/cloud /autonomous/occupancy_grid /object_detector/markers /ar_tracker/tags goal_manager/confirmed_targets -s mcap -b 500000000"
