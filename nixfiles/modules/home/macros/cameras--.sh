#!/usr/bin/env bash

set -euo pipefail

# this is the serial that cameras2 prints out
name=$1

# this is the /dev/videoX node for the camera
node=$(/home/nova/Builds/master/bin/ros2 topic echo /camera_directory/cameras --once --timeout 1 | grep -A 1 "$name" | tail -n1 | cut -d: -f2 | tr -d " ")

if [ "a$node" = a ]; then
  echo "Couldn't find that camera from /camera_directory/cameras"
  exit 1
fi

formats=$(v4l2-ctl --list-formats -d "$node" | grep "\[[0-9]\]" | cut -d "'" -f 2)

if echo "$formats" | grep H264; then
	filter="video/x-h264"
	extra="rtph264pay ! rtph264depay" # the wide angle cams we have don't work without this. Don't know why.
	#extra='h264parse ! "video/x-h264,stream-format=avc,alignment=au'
elif echo "$formats" | grep MJPG; then
	filter="image/jpeg"
	extra='decodebin ! videoconvert'
else
	# Do we have h265? or any other cases?
	filter="video/x-raw"
	extra='videoconvert'
fi

# appease shellcheck because telling it to ignore a rule doesn't work
read -ra extra_array <<< "${extra}"

echo Starting gstreamer...
echo gst-launch-1.0 v4l2src device="$node" ! "$filter" ! "${extra_array[@]}" ! webrtcsink meta=\""meta, serial=(string)$name"\"
exec gst-launch-1.0 v4l2src device="$node" ! "$filter" ! "${extra_array[@]}" ! webrtcsink meta="meta, serial=(string)$name"
