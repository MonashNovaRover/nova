#! /usr/bin/env bash

if [ "$1" == "on" ]; then
    echo "Cop Mode Enabled"
    while true; do
        cansend can0 095#0100
        sleep 0.3
        cansend can0 095#0300
        sleep 0.3
    done
elif [ "$1" == "off" ]; then
    echo "Cop Mode Disabled"
    cansend can0 091#0000
else
    echo "Usage: $0 {on|off}"
    exit 1
fi
