#! /usr/bin/env bash

# This script zeroes out the pivot for the chosen BLCMD.

echo "Ensure that drive is not running before you continue."
echo

while true; do
  read -p "BLCMD ID: " blcmd_id

  if [[ "$blcmd_id" =~ ^[1-8]$ ]]; then
    break
  else
    echo "Invalid input. Use the 'list-blcmds' command to view all BLCMD IDs."
  fi
done

echo "Please manually swivel the pivot to ensure the wheel is straight."
read -p "Press any key to continue." -n1 -s
echo

echo "cansend in progress..."
echo

# Zero pivot
cansend can0 0${blcmd_id}8#
sleep 0.5
cansend can0 0${blcmd_id}8#

sleep 1

# Move to angle
cansend can0 0${blcmd_id}4#c764
sleep 0.5
cansend can0 0${blcmd_id}4#c764

sleep 1

# Zero pivot
cansend can0 0${blcmd_id}8#
sleep 0.5
cansend can0 0${blcmd_id}8#

echo
echo "Done."
