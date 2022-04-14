#!/bin/bash

# +--------------------------------------------+
#               MONASH NOVA ROVER
# +--------------------------------------------+
#
# This script runs CAN, either virtually or on
#   a real bus. This must be executed before any
#   of the CAN modules can be used.
#
# Some examples could be:
#   can start can0
#   can stop vcan0
#
# The options are:
#   can0 -  CAN 1 Line
#   can1 -  CAN 2 Line
#   all -   CAN 1 and CAN 2 Line
#   vcan0 - Virtual CAN 1 Line
#   vcan1 - Virtual CAN 2 Line
#
# +--------------------------------------------+

# Add the colors
ERROR='\033[0;31;1m'
END='\033[0m'

# Create an error function
information () {
    printf "${ERROR}${1}${END}\n"
}

# Reset the failed flag
failed="0"


# Check if the first keyword command
if [[ $1 = "start" ]]
then
    command="start"
elif [[ $1 = "stop" ]] 
then
    command="stop"
else
    information "Invalid Command! Please enter 'start' or 'stop'."
    failed="1"
fi


# Get the CAN line
if [[ $2 = "can0" ]]
then
    can="can0"
elif [[ $2 = "can1" ]]
then
    can="can1"
elif [[ $2 = "vcan0" ]]
then
    can="vcan0"
elif [[ $2 = "vcan1" ]]
then
    can="vcan1"

# In the case of starting all CAN lines
elif [[ $2 = "all" ]]
then
    can start can0
    can start can1
    failed="1"

# If invalid argument
else
    information "Incorrect CAN line. Please enter one of:\n\t['can0', 'can1', 'all', 'vcan0', 'vcan1']"
    failed="1"
fi


# If the bitrate parameter exists
if [[ -z $3 ]]; then
    bitrate=200000   # Default
else
    bitrate=$3
fi


# Print message
if [[ $failed != "1" ]]
then
    echo "Attempting to $command $can..."
fi

# Check if it is virtual CAN
if [[ ${can:0:1} == "v" && $failed != "1" ]]
then

    # Run the Virtual CAN
    sudo modprobe vcan

    # If turning on CAN
    if [[ $command == "start" ]]
    then

        # Create the CAN link
        sudo ip link add dev ${can:1:5} type vcan
        sudo ip link set up ${can:1:5}

    # If turning off CAN
    else

        sudo ip link set down ${can:1:5}

    fi

elif [[ $failed != "1" ]]
then

    # Run the CAN
    sudo modprobe can
    sudo modprobe can-raw
    sudo modprobe mttcan

    # If turning on CAN
    if [[ $command == "start" ]]
    then

        # Create the CAN link
        sudo ip link set $can type can bitrate $bitrate berr-reporting on 
        sudo ip link set up $can

    # If turning off CAN
    else

        sudo ip link set down $can

    fi
fi
