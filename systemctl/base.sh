#!/bin/bash
# Go to home directory 
cd ~/

# Source development environment
source ./nova_ws/install/setup.bash

# Check if screen session already exists (prevents duplicate roslaunch calls)
if ! screen -list | grep -q "base_launch"; then
	screen -dmS "base_launch" bash -c 'ros2 launch core base.launch.py'
else
    echo "base_launch already running!"
fi
