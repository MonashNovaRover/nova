# +----------------------+
#    MONASH NOVA ROVER
# +----------------------+
#
# Core Repository Bash Script
# This should be added to everyone's .bashrc file
#       sudo echo "source ~/nova_ws/src/core" < ~/.bashrc
# This will initialise all macros and set up ROS correctly
# 
# +----------------------+

# Macro Aliases
alias build='. ~/nova_ws/src/core/macros/build.sh'    # Runs colcon build
alias setup='. ~/nova_ws/install/setup.bash'          # Sets up the ROS repository when scripts change

# Directory Aliases
alias nova='cd ~/nova_ws'
alias core='cd ~/nova_ws/src/core'

# Calls bash executions
setup                                             # Sets up the bash file
source /opt/ros/eloquent/setup.bash               # Sources ROS 2 Eloquent
. ~/nova_ws/install/setup.bash                    # Runs the workspace setup file
