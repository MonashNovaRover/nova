#!/bin/bash

# WARNING: this should only be run on Jetson on the rover (not on a personal computer)
# setting up the base service

for name in base rover arm; do
    echo "Setting up the ${name} service..."
    echo "Placing service in /etc/systemd/system/$name.service"
done
