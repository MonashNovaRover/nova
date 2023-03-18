#!/bin/bash

# Set CPU configuration to always run CPUs at max clock speed
sudo bash -c "sleep 20"
/usr/sbin/nvpmodel -m 0
/usr/bin/jetson_clocks
