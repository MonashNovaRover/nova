#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# This script allows users to connect to WiFi
#   devices using the command line for helping
#   with the Jetson.
#
# The commands exists like:
#
# > wifi rescan
# > wifi list
# > wifi connect [DEVICE] [PASSWORD]
#
# +--------------------------------------------+

# Add the colors
ERROR='\033[0;31;1m'
END='\033[0m'

# Create an error function
error () {
    printf "${ERROR}${1}${END}\n"
}

# Checks for missing keyword
if [[ -z $1 ]]
then
    # Display error
    error "Missing command. Please select 'rescan', 'list' or 'connect'."

# Checks for rescan command
elif [[ $1 = "rescan" ]]
then
    sudo nmcli device wifi rescan

# Checks for list command
elif [[ $1 = "list" ]]
then
    sudo nmcli device wifi rescan
    sudo nmcli device wifi list

# Checks for connect command
elif [[ $1 = "connect" ]]
then
    # Checks for missing device
    if [[ -z $2 ]]
    then
        error "Missing device name."
    
    # Check if password missing
    elif [[ -z $3 ]]
    then
        sudo nmcli device wifi connect $2
    
    # Assume password is third parameter
    else
        sudo nmcli device wifi connect $2 password $3
    
    fi

# Display invalid command
else
    error "Invalid WiFi command."
fi
