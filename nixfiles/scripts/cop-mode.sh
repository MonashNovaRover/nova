#! /usr/bin/env bash

PID_FILE="/tmp/cop_mode.pid"

if [ "$1" == "on" ]; then
    echo "Cop Mode Enabled"
    
    # Prevent multiple instances
    if [ -f "$PID_FILE" ]; then
        echo "Cop Mode already active!"
        exit 1
    fi

    # Run loop in the background and save PID
    ( while true; do
        cansend can0 095#0100
        sleep 0.3
        cansend can0 095#0300
        sleep 0.3
    done ) &

    echo $! > "$PID_FILE"  # Save PID of background process

elif [ "$1" == "off" ]; then
    echo "Cop Mode Disabled"

    if [ -f "$PID_FILE" ]; then
        kill "$(cat "$PID_FILE")" 2>/dev/null
        rm -f "$PID_FILE"
    fi

    cansend can0 091#0000
else
    echo "Usage: $0 {on|off}"
    exit 1
fi
