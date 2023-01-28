#!/bin/bash

# Check if the first keyword command
if [[ $1 = "base" ]]

then

    bash -c "sudo systemctl start base" 

elif [[ $1 = "arm" ]] 

then

    bash -c "sudo systemctl start arm" 

elif [[ $1 = "rover" ]] 

then

    bash -c "sudo systemctl start rover" 

else

    echo "Invalid Command! Please enter 'rover', 'arm' or 'base'."

fi

