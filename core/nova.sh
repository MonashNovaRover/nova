#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# Core Repository Bash Script
# This should be added to everyone's .bashrc file
#       sudo echo "source ~/nova_ws/src/rover/core" < ~/.bashrc
# This will initialise all macros and set up ROS correctly.
#
# +--------------------------------------------+

# Sources the correct ROS bash file
source ~/nova_ws/src/rover/core/macros/ros.sh
python3 ~/nova_ws/src/rover/core/macros/prompt.py



# appending to pythonpath for autonomous folders
<<<<<<< HEAD:core/nova.sh
export PYTHONPATH=$PYTHONPATH:~/nova_ws/src/rover/autonomous/autonomous
export PYTHONPATH=$PYTHONPATH:~/nova_ws/install/autonomousrlib/python3.8/site-packages
export PYTHONPATH=$PYTHONPATH:~/nova_ws/rover/src/gui/gui/
=======
export PYTHONPATH=$PYTHONPATH:~/nova_ws/src/autonomous/autonomous
export PYTHONPATH=$PYTHONPATH:~/nova_ws/install/autonomous/lib/python3.8/site-packages
export PYTHONPATH=$PYTHONPATH:~/nova_ws/install/autonomous/lib/python3.6/site-packages
export PYTHONPATH=$PYTHONPATH:~/nova_ws/src/gui/gui/
>>>>>>> replaced typo and added support for both python3.6 and 3.8 pybind files:nova.sh

# Source the aliases (if ROS 2)
if [[ $ROS_VERSION -eq 2 ]]; then
    source ~/nova_ws/src/rover/core/macros/alias.sh
fi

# set the location of .vimrc
export VIMINIT="source ~/nova_ws/src/rover/core/settings/.vimrc"
