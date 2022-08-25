#!/bin/bash

# start can as if we were root (we need to be root for the system ctl service to work
sudo bash -c "/home/nova/nova_ws/src/core/macros/can.sh start can0"
sudo bash -c "/home/nova/nova_ws/src/core/macros/can.sh start can1"
