# Cameras 2++
Rewrite of Cameras 2 in c++, works the same as cameras 2
Split into two parts:

## camera_directory_service
Uses Video for Linux (v4l) libraries to detect connected video capture devices and then relays this information to the ros topic: `/camera_directory/cameras` with a camera's serial and `/dev/` node

Also listens to `/camera_directory/discover` to retrigger parameter read and camera discovery. 

Otherwise it will automatically retrigger discovery (But **not** parameter read) every 5000ms.

## camera_streamer_service
Listens to the `/camera_directory/cameras` topic and creates a gstreamer pipeline for each camera utilising gstreamer to (by default) make a pipeline starting with v4lsrc and ending at webrtcsink resulting in video feeds being decoded and then recoded for webrtc.

More pipelines can be created and coded!

Listens to `/camera_streamer/stream/*` where * can be:
- `start` - starts a camera's pipeline
- `stop` - stops a camera's pipeline and deletes it
- `pause` - pauses a camera's pipeline which can be restarted
All utilise the CameraOperation service and can handle multiple camera serials.

Currently `/camera_streamer/stream/get_stats` and `/camera_streamer/get_host_ip` are **not** implemented (but their python code exists as a comment in the node).

### Build
`nix-build nova/nixfiles -A pkgs.ros.cameras`\
 or `ws-build`

### Run
`result/bin/ros2 launch cameras cameras.launch.py`\
or individually\
`result/bin/ros2 run cameras camera_directory_service`\
`result/bin/ros2 run cameras camera_streamer_service`\
`result/bin/gst-webrtc-signalling-server`\
then run the `nova-gui` \
or clone the gst-plugins-rs repo (v0.10.2) and run:\
`cd gst-plugins-rs/net/webrtc`\
`python3 -m http.server -d www/`


### Development in VSCode
I developed on wsl in vscode here's how i setup my ide:
Extensions:
- ms-vscode.cpptools
- twxs.cmake
- ms-vscode.cmake-tools
- bbenoist.Nix
- ms-python.python
- ms-python.vscode-pylance

Workflow:\
`nix-shell nova/nixfiles -A pkgs.ros.cameras`\
`cd src/ros/cameras/cameras2++`\
`code .`
Now you can start editing :)
