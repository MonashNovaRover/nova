#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# Sets up all of the repositories and the workspace
#   for the MNR ROS code. Make sure that this is
#   the first file that you execute when setting
#   your machine up to run Nova code.
#
# Prior to running this code, make sure you have
#   been added to the MonashNovaRover GitHub
#   organisation. You must be a member to have
#   access to this code, and will need to make
#   sure you have SSH-key security enabled on
#   your device. Failing to do so may result in
#   errors when running this script. 
#
# +--------------------------------------------+

# Create the workspace
mkdir -p ~/nova_ws/src
cd ~/nova_ws
colcon build

# Clone the ROS GitHub files
cd ~/nova_ws/src
git clone https://github.com/MonashNovaRover/autonomous.git
git clone https://github.com/MonashNovaRover/cameras.git
git clone https://github.com/MonashNovaRover/control.git
git clone https://github.com/MonashNovaRover/core.git
git clone https://github.com/MonashNovaRover/electronics.git
git clone https://github.com/MonashNovaRover/gui.git
git clone https://github.com/MonashNovaRover/science.git

# Clone the other GitHub files
mkdir -p ~/nova_ws/other
cd ~/nova_ws/other
git clone https://github.com/MonashNovaRover/arduinos.git
git clone https://github.com/MonashNovaRover/pics.git
git clone https://github.com/MonashNovaRover/ik_machine.git

# Add the nova.sh bash script to the bashrc
sudo echo "source ~/nova_ws/src/core/nova.sh" >> ~/.bashrc
source ~/.bashrc

# Build the workspace
cd ~/nova_ws
colcon build
