#!/bin/bash

# Check if the first keyword command
if [[ $1 = "base" ]]

then

    bash -c "sudo systemctl stop base.service" 

elif [[ $1 = "arm" ]] 

then

    bash -c "sudo systemctl stop arm.service" 

elif [[ $1 = "rover" ]] 

then

    bash -c "sudo systemctl stop rover.service" 

else

    echo "Invalid Command! Please enter 'rover', 'arm' or 'base'."

fi

