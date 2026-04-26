#! /usr/bin/env bash
# echo "Current USB connections:"
# ls /dev/ttyACM*
# echo "Please plug in drone controller"
# read -p "Press any key to continue..." -n1 -s
# echo
# echo "New USB connections:"
# ls /dev/ttyACM*
# read -p "Which number is the controller? (0, 1, 2 ...): " usb_id
# echo
# echo
# ~/Builds/drone/bin/mavproxy.py --master=/dev/ttyACM{usb_id} --baudrate 115200 --out=udp:127.0.0.1:14550 --out=udp:127.0.0.1:14551
# ~/Builds/master/bin/mission-planner
# ~/Builds/master/bin/ros2 run drone_gps drone_gps.py