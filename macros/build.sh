#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# Build script builds the ROS workspace.
# This can be called from any script and any
#   package.
# To build one package, add the package name as 
#   an argument to the console script.
#
#           e.g 'build core'
#
# +--------------------------------------------+

cwd=$(pwd);     # Save the current directory
cd ~/nova_ws;   # Navigate to the workspace directory
setup           # Call the setup macro

# Reinstall the communications utility package
cd ~/nova_ws/other/coms_utils
python3 setup.py install --user &> /dev/null

# Change back to the root directory
cd ~/nova_ws

# Check if a keyword used
if [[ -z $1 ]]; then
    colcon build;   # Build the workspace

# Check for clean
elif [[ $1 = "clean" ]]
then
    rm -r build install log
    colcon build

# Build only a certain package
else
    colcon build --packages-select $1
fi

cd $cwd;        # Return back to the previous directory
