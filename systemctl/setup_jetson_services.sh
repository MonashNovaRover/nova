#!/bin/bash

# WARNING: this should only be run on Jetson on the rover (not on a personal computer)
# setting up the base service
sudo apt update
sudo apt install screen

for name in base rover arm can; do
    echo "Setting up the ${name} service... "
    echo "Placing service in /etc/systemd/system/${name}.service"
    sudo cp ${name}.service /etc/systemd/system/${name}.service
    sudo systemctl daemon-reload
    sudo systemctl enable ${name}.service
done

sudo systemctl daemon-reload

echo "All services have been setup. Restart Jetson and follow terminal instructions for how to view running processes."
