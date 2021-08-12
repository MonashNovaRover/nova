#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# Build script builds the ROS workspace.
# This can be called from any script and any
#   package.
#
# +--------------------------------------------+

cwd=$(pwd);     # Save the current directory
cd ~/nova_ws;   # Navigate to the workspace directory
colcon build;   # Build the workspace
cd $cwd;        # Return back to the previous directory
