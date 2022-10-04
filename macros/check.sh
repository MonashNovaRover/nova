#!/bin/bash

# Check if the first keyword command
if [[ $1 = "base" ]]

then

    bash -c "screen -r base" 

elif [[ $1 = "arm" ]] 

then

    bash -c "screen -r arm"

elif [[ $1 = "rover" ]] 

then

    bash -c "screen -r rover"

else

    echo "Invalid Command! Please enter 'rover', 'arm' or 'base'."

fi

