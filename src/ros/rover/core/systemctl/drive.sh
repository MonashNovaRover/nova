#!/bin/bash
cd ~/

# Source development environment
source ./nova_ws/install/setup.bash

# Check if screen session already exists (prevents duplicate roslaunch calls)
if ! screen -list | grep -q "drive_launch"; then
	screen -dmS "drive_launch" bash -c 'sleep 20 && ros2 launch core drive.launch.py'
else
    echo "drive_launch already running!"
fi
