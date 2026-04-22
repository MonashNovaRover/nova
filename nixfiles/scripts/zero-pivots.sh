#! /usr/bin/env bash

# This script zeroes out the pivot for the chosen BLCMD.

# BLCMD IDs
declare -A pivots
pivots=(
  [5]=FLP
  [6]=BLP
  [7]=BRP
  [8]=FRP
)

echo "Ensure that drive is not running before starting the alignment process."
echo

while true; do
  echo "Please enter a BLCMD ID (5-8). To zero all pivots, enter '0' instead."
  read -p "BLCMD ID: " blcmd_id

  if [[ "$blcmd_id" =~ ^(0|[5-8])$ ]]; then
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

read -p "Press any key to continue. " -n1 -s
echo

echo "Initiating cansend..."
echo

zero_pivot() {
  local id=$1
   # Zero pivot
  cansend can0 0${id}B#
}

if [[ "$blcmd_id" == "0" ]]; then
  for id in $(echo "${!pivots[@]}" | tr ' ' '\n' | sort); do
    pivot=${pivots[$id]}
    zero_pivot "$id"
    echo "BLCMD ${id} (${pivot}) successfully aligned."
  done
else
  zero_pivot "$blcmd_id"
  echo "BLCMD ${blcmd_id} (${pivots[$blcmd_id]}) successfully aligned."
fi

echo
echo "Done."