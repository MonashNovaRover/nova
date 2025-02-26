#! /usr/bin/env bash

while true; do
    cansend can0 095#0100
    sleep 0.3
    cansend can0 095#0300
    sleep 0.3
done
