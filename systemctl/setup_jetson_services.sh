#!/bin/bash

# WARNING: this should only be run on Jetson on the rover (not on a personal computer)
# setting up the base service

for name in base rover arm can; do
    echo "Setting up the ${name} service... "
    echo "Placing service in /etc/systemd/system/${name}.service"
    sudo cp ${name}.service /etc/systemd/system/${name}.service
    sudo systemctl daemon-reload
    sudo systemctl enable ${name}.service
done

