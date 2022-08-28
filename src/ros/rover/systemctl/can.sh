#!/bin/bash

# start can as if we were root (we need to be root for the system ctl service to work

# note that we can't use `can start all` because systemd is bad at aliases
sudo bash -c "/home/nova/nova_ws/src/core/macros/can.sh start can0"
sudo bash -c "/home/nova/nova_ws/src/core/macros/can.sh start can1"
