#! /usr/bin/env bash

# This script zeroes each joint of the arm (J1-J6).

# Joint IDs
declare -A joints
joints=(
  [1]="04"
  [2]="08"
  [3]="0C"
  [4]="10"
  [5]="14"
  [6]="18"
)

echo "Ensure that the arm is not running before starting the alignment process."
echo

while true; do
  echo "Select a joint to zero (J1-J6)."
  echo "Otherwise, enter '0' to zero all joints."
  read -p "Joint ID: " joint_id

  if [[ "$joint_id" =~ ^(0|[1-6])$ ]]; then
    break
  else
    echo
    echo "Invalid input. Please enter a valid integer."
    echo
  fi
done

echo

zero_joint() {
  local joint_number=$1
  local joint_id=${joints[$joint_number]}

  # Zero the joint
  cansend can1 "0A3#${joint_id}"
  sleep 0.5
  cansend can1 "0A3#${joint_id}"
  sleep 1

  echo "J${joint_number} successfully aligned."
}

if [[ "$joint_id" == "0" ]]; then
  echo "Please manually move each joint to the zero position."
  echo "This can be done with the joysticks, or by using the twitch kit."
  read -p "Press any key to continue. " -n1 -s
  echo
  echo
  echo "Initiating cansend..."
  
  for joint in {1..6}; do
    zero_joint "$joint"
  done
else
  echo "Please manually move J${joint_id} to the zero position."
  echo "This can be done with the joysticks, or by using the twitch kit."
  read -p "Press any key to continue. " -n1 -s
  echo
  echo
  echo "Initiating cansend..."
  
  zero_joint "$joint_id"
fi

echo
echo "Done."
echo
echo "Remember to reset the arm to its regular stowed position."
