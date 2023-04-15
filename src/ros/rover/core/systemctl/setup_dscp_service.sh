#!/bin/bash

#$ Sets up the dscp service to set ros messages to higher priorities over the radio

for name in dscp; do
    echo "Setting up the ${name} service... "
    echo "Placing service in /etc/systemd/system/${name}.service"
    echo "Placing executable in /etc/systemd/system/${name}.sh"
    # Find and replace with this user's home directory
    sed -i 's/HOMEDIR/\/home\/'$USER'/' ${name}.service
    sudo cp ${name}.service /etc/systemd/system/${name}.service
    sudo cp ${name}.sh /usr/sbin/${name}.sh
    sudo systemctl daemon-reload
    sudo systemctl enable ${name}.service
done

sudo systemctl daemon-reload

