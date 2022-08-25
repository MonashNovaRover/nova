#!/bin/bash
# Go to home directory 
cd ~/

# Source development environment
source ./nova_ws/install/setup.bash

# Check if screen session already exists (prevents duplicate roslaunch calls)
if ! screen -list | grep -q "arm_launch"; then
	screen -dmS "arm_launch" bash -c 'sleep 3 && ros2 launch core arm.launch.py'
else
    echo "arm_launch already running!"
fi
