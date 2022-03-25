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

# Add the colors
TITLE='\033[0;36;1m'
INFO='\033[0;32;1m'
END='\033[0m'

# Create a print function
title () {
    echo 
    echo "------------------------------"
    printf "${TITLE}${1^^}${END}\n"
    echo "------------------------------"
    echo
}

# Create an information function
information () {
    printf "\n${INFO}${1}${END}\n"
}


# Prompt the user to run program
title "NOVA ROVER INSTALLATION SCRIPT"
echo "Make sure you have set up your Git account before starting this installation."
echo "   This will include having a correct SSH key associated with your MNR account."
echo "This script will delete the current nova_ws folder, if it exists."
echo "Enter (Y)es to confirm or any key to cancel."
read confirmation
if [[ "$confirmation" != "y"  &&  "$confirmation" != "Y" ]]; then
    echo "Cancelling Installation."
    exit 1;
fi

echo "Would you like to skip dependency installation? (Y) to skip."
read dependencies

# Check if requiring to install dependencies
if [[ "$dependencies" != "y" && "$dependencies" != "Y" ]]; then

# First step is to install dependencies and packages
title "Installing Dependencies"
sudo apt update -y

# Install Git
sudo apt install git -y

# Installing ROS 2
information "Installing ROS 2..."
sudo apt update && sudo apt install curl gnupg2 lsb-release -y
curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo apt-key add -
sudo sh -c 'echo "deb [arch=$(dpkg --print-architecture)] http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main" > /etc/apt/sources.list.d/ros2-latest.list'
sudo apt update -y
sudo apt install ros-eloquent-desktop -y
source /opt/ros/eloquent/setup.bash
sudo apt install -y python3-pip -y
pip3 install -U argcomplete -y
sudo apt install -y python-rosdep -y
sudo rosdep init -y
rosdep update -y
sudo apt install python3-colcon-common-extensions -y

# Installing Text Editors
information "Installing Editors..."
sudo apt-get -y install nano
sudo apt install vim -y
sudo apt install screen -y

# Install C++
information "Installing C++..."
sudo apt install build-essential -y
sudo apt-get install manpages-dev -y
sudo apt install libudev-dev -y

# Installing Cameras and GStreamer
information "Installing Cameras..."
sudo apt-get -y install gstreamer-1.0 python-gi gstreamer1.0-tools gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-dev python-gst-1.0 -y
sudo apt-get install gstreamer1.0-plugins-ugly -y
sudo apt install v4l-utils -y
pip3 install requests -y

# Installing GUI tools
information "Installing GUI Tools..."
sudo apt-get -y install nodejs-dev node-gyp libssl1.0-dev
sudo apt-get -y install npm
curl -fsSL https://deb.nodesource.com/setup_current.x | sudo -E bash -
sudo apt-get install -y nodejs
sudo npm install -g npm@7.11.2
sudo pip3 install flask_cors
sudo pip3 install flask_socketio
sudo pip3 install flask_cors

# Installing Net Tools
information "Installing Networking..."
sudo apt -y install net-tools
sudo apt -y install can-utils
sudo apt -y install exfat-fuse exfat-utils
sudo gpasswd --add ${USER} dialout
pip3 install python-can==3.3.4

# Installing IK and the arm tools
sudo apt install libeigen3-dev libcppunit-dev -y
cd ~
git clone git@github.com:orocos/orocos_kinematics_dynamics.git
mkdir -p orocos_kdl_builds/build.1.5.1  # Modify version number as needed
cd orocos_kdl_builds/build.1.5.1
cmake ~/orocos_kinematics_dynamics/orocos_kdl
make
sudo make install
cd ~
rm -rf orocos_kinematics_dynamics

# ---------------------------------------- #

# End dependencies
fi

# Adding Git permissions
information "Setting up Git..."
printf "Please enter your Git User Name: "
read username
printf "Please enter your Git Email: "
read email
git config --global user.name "$username"
git config --global user.email "$email"

# Create the workspace
information "Creating Nova Workspace..."
sudo rm -rf ~/nova_ws
mkdir -p ~/nova_ws/src
sudo chown -R $USER:$USER ~/nova_ws
cd ~/nova_ws
colcon build

# Clone the ROS GitHub files
information "Cloning Repositories..."
cd ~/nova_ws/src

# Check if the SSH key exists
if [ ! -f ~/.ssh/id_ed25519.pub ]; then
    # Create the keygen
    sudo ssh-keygen -t ed25519 -C "$email"

    echo ""

    # Display the SSH key
    sudo cat ~/.ssh/id_ed25519.pub

    printf "Please copy your SSH key above to your GitHub account..."

    read empty
fi

# Clones all folders
git clone git@github.com:MonashNovaRover/autonomous.git
git clone git@github.com:MonashNovaRover/cameras.git
git clone git@github.com:MonashNovaRover/control.git
git clone git@github.com:MonashNovaRover/core.git
git clone git@github.com:MonashNovaRover/electronics.git
git clone git@github.com:MonashNovaRover/gui.git
git clone git@github.com:MonashNovaRover/science.git

# Fix autonomous installs
cd ~/nova_ws/src/autonomous
git submodule update --init --recursive

# Clone the other GitHub files
mkdir -p ~/nova_ws/other
cd ~/nova_ws/other
git clone git@github.com:MonashNovaRover/arduinos.git
git clone git@github.com:MonashNovaRover/pics.git
git clone git@github.com:MonashNovaRover/ik_machine.git
git clone git@github.com:MonashNovaRover/coms_utils.git

# Add the nova.sh bash script to the bashrc
information "Setting up Workspace..."
sudo echo "source ~/nova_ws/src/core/nova.sh" >> ~/.bashrc
source ~/nova_ws/src/core/nova.sh

# Build the workspace
cd ~/nova_ws
export CMAKE_PREFIX_PATH=""
export AMENT_PREFIX_PATH=""
colcon build

# Building the GUI
cd ~/nova_ws/src/gui/wombatx
npm install
npm update

# Completed
title "Installation Complete!"
echo "All Nova files are now located in ~/nova_ws in your home directory."
cd ~/nova_ws
