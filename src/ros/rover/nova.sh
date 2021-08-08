# Sourcing ROS
source /opt/ros/eloquent/setup.bash               # Sources ROS 2 Eloquent
. ~/nova_ws/install/setup.bash                    # Runs the workspace setup file

# Macro Aliases
alias build='. ~/nova_ws/core/macros/build.sh'    # Runs colcon build

# Directory Aliases
alias nova='cd ~/nova_ws'
alias core='cd ~/nova_ws/core'
