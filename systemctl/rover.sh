#!/bin/bash
cd ~/

# Source development environment
source ./nova_ws/install/setup.bash

# Check if screen session already exists (prevents duplicate roslaunch calls)
if ! screen -list | grep -q "rover_launch"; then
	# New command with logging, and ROBOT environment variable
	screen -dmS "rover_launch" bash -c 'sleep 2 && ros2 launch core rover.launch.py'
else
    echo "rover_launch already running!"
fi
