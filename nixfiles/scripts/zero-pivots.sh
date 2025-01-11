#! /usr/bin/env bash

# This script zeroes out the pivot for the chosen BLCMD.

echo "Ensure that drive is not running before starting the alignment process."
echo

while true; do
  echo "Please enter a BLCMD ID (1-8). To zero all pivots, enter '0' instead."
  read -p "BLCMD ID: " blcmd_id

  if [[ "$blcmd_id" =~ ^[0-8]$ ]]; then
    break
  else
    echo "Invalid input. Use the 'list-blcmds' command to view all BLCMD IDs."
  fi
done

echo

if [[ "$blcmd_id" == "0" ]]; then
  echo "Please manually swivel each pivot to ensure all wheels are straight."
else
  echo "Please manually swivel the selected pivot to ensure the wheel is straight."
fi

read -p "Press any key to continue." -n1 -s
echo

echo "Initiating cansend..."
echo

zero_pivot() {
  local id=$1
   # Zero pivot
  cansend can0 0${id}8#
  sleep 0.5
  cansend can0 0${id}8#
  
  sleep 1
  
  # Move to angle
  cansend can0 0${id}4#c764
  sleep 0.5
  cansend can0 0${id}4#c764
  
  sleep 1
  
  # Zero pivot
  cansend can0 0${id}8#
  sleep 0.5
  cansend can0 0${id}8#
}

if [[ "$blcmd_id" == "0" ]]; then
  for id in 5 6 7 8; do
    zero_pivot "$id"
    echo "BLCMD ${id} successfully aligned."
  done
else
  zero_pivot "$blcmd_id"
  echo "BLCMD ${blcmd_id} successfully aligned."
fi

echo
echo "Done."
