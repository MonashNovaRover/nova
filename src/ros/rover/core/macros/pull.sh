#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# Pulls all GitHub code into all of the repositories
#   found in the workspace src folder.
#
# +--------------------------------------------+

cwd=$(pwd);         # Save the current directory
cd ~/nova_ws/src;   # Navigate to the Nova workspace

# Find all the files and pull
find . -mindepth 1 -maxdepth 1 -type d -print -exec git -C {} pull \;

cd ~/nova_ws/other;   # Navigate to the other repositories workspace

# Find all the files from other repos and pull
find . -mindepth 1 -maxdepth 1 -type d -print -exec git -C {} pull \;

cd $cwd;            # Return back to the previous directory
