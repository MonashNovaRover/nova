#!/bin/bash

#$ Sets up the dscp service to set ros messages to higher priorities over the radio

for name in dscp; do
    echo "Setting up the ${name} service... "
    echo "Placing service in /etc/systemd/system/${name}.service"
    # Find and replace with this user's home directory
    cp ${name}_template.service ${name}.service
    sed -i 's/HOMEDIR/\/home\/'$USER'/' ${name}.service
    sudo mv ${name}.service /etc/systemd/system/${name}.service
    sudo systemctl daemon-reload
    sudo systemctl enable ${name}.service
done

sudo systemctl daemon-reload

