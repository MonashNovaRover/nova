#!/bin/bash

# check if the can start service exists - this is a reasonable test for if we are on a Jetson 
# or on a laptop.
if [ -f "/etc/systemd/system/can.service" ]
then

    echo "Hello! It seems like you are logged into a Jetson!"
    echo "Please read ALL of the below before continuing..."
    
    echo "" 

    echo "To check the currently running rover launch file, type check rover"
    echo "To check the currently running arm launch file, type check arm"
    echo "To check the currently running base launch file, type check base"
    
    echo ""

    echo "The above commnds will take you to a screen session. To exist the screen session, use CRTL+A, D"

    echo "" 

    echo "If any of these don't take you to a new screen, they have probably crashed and will need to be run again!"
    echo "Simply type restart system (where system is either rover, arm or base"

else

    echo "Hello! It seems like you are logged into a laptop!"
    echo "If you are intending to use this as a base station, make sure to do the following: "
    
    echo "1. ssh into to the required jetson"
    echo "2. Follow provided instructions to check the status of running services"
    echo "3. run stop_base to stop base, and run it on your laptop"

fi
