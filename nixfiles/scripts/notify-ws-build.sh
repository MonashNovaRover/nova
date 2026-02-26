#!/usr/bin/env

# This script should be run when the laptop is connected to the internet
if ping -c1 8.8.8.8 > /dev/null; then

  # Check how git branches are different
  cd ~/nova
  before=$(cat ~/Builds/master/nova-git-metadata | head -n 1 | cut -d " " -f2)
  git fetch origin > /dev/null
  after=$(git rev-parse origin/master)

  if [[ "$before" != "$after" ]]; then
    notify-send "ws-build" "Is not recent, please build"
  else
    notify-send "ws-build" "Is up to date"
  fi
fi 
