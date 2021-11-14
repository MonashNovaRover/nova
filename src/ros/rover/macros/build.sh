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

# Check if a keyword used
if [[ -z $1 ]]; then
    colcon build;   # Build the workspace
else
    colcon build --packages-select $1
fi

cd $cwd;        # Return back to the previous directory
