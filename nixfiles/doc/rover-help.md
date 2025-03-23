=========================
RUNNING THE ROVER
=========================

# Connecting
Type 'jetson' to SSH into the rover (via ethernet or base station).
If you're on the makerspace wifi, you'll need to use 'J1', 'J2', 'J3', or 'N1' instead (case sensitive). If these aliases don't work, the full commands are listed below.

J1: ssh nvidia@10.0.2.21
J2: ssh nvidia@10.0.2.22
J3: ssh nvidia@10.0.2.23

N1: ssh nova@10.0.2.11
N2: ssh nova@10.0.2.12
N3: ssh nova@10.0.2.13

Radios (Jetson): ssh nvidia@10.0.0.10
Radios (Orin): ssh nova@10.0.0.11

=========================

# Base
Launch this on the metabox.
Try the 'launch-base' alias, otherwise use the command below.
~/Builds/master/bin/ros2 launch nova_bringup base.launch.py

# Drive
Launch this on the jetson.
Try the 'launch-drive' alias, otherwise use the command below.
~/Builds/master/bin/ros2 launch nova_bringup drive.launch.py

=========================

# GUI
Launch the following on the metabox.

In one terminal:
1. Try the 'gui-shell' alias otherwise:
     nova-shell -A pkgs.ros.nova-gui
2. Try the 'gui-run' alias otherwise:
     cd ~/nova/src/ros/nova-gui/nova-gui
     yarn dev

In another terminal:
1. Try the 'gui-rosbridge' alias, otherwise: 
    ~/Builds/master/bin/ros2 launch rosbridge_server rosbridge_websocket_launch.xml

=========================

# Arm
Launch this on the jetson.
When running the arm payload, you DO need to run drive.
Try the 'launch-arm' alias, otherwise use the command below.
~/Builds/master/bin/ros2 launch nova_bringup arm.launch.py

# C&E
Launch this on the jetson.
When running the excavation & construction payload, you DON'T need to run drive.
Try the 'launch-ec' alias, otherwise use the command below.
~/Builds/master/bin/ros2 launch nova_bringup ec_rover.launch.py

# Science (ARC)
Launch this on the jetson.
When running the science payload, you DO need to run drive.
Try the 'launch-science-arc' alias, otherwise use the command below.
~/Builds/master/bin/ros2 launch nova_bringup arc_science.launch.py

# Science (URC)
Launch this on the jetson.
When running the science payload, you DO need to run drive.
Try the 'launch-science-urc' alias, otherwise use the command below.
~/Builds/master/bin/ros2 launch nova_bringup urc_science.launch.py

=========================

# Cameras
Launch this on the jetson
Replace '?' with either 'arm', 'ec', or 'arc_science'.
launch-cameras payload:=?

if that doesn't work try:
cameras-legacy payload:=?

# Reolink Camera
First, run the command below.
nix-shell -p mpv

Use the 'reolink-low' alias to run the camera in low quality, low latency mode. Otherwise, try the command below.
mpv --rtsp-transport=udp --no-cache --untimed --video-sync=display-resample --deinterlace=no --profile=low-latency --demuxer-max-bytes=512K --demuxer-max-back-bytes=512K rtsp://admin:Lab188b37@10.0.1.100:554/h264Preview_01_sub

Use the 'reolink-high' alias to run the camera in high quality, high latency mode. Otherwise, try the command below.
mpv --rtsp-transport=udp --no-cache --untimed --video-sync=display-resample --deinterlace=no --profile=low-latency --demuxer-max-bytes=1M --demuxer-max-back-bytes=1M rtsp://admin:Lab188b37@10.0.1.100:554/h264Preview_01_main

Access the camera at the web address below, using the password used for the workshop computers.
http://10.0.1.100

=========================

# LEDs
leds-red: cansend can0 095#0100
leds-green: cansend can0 095#0200
leds-blue: cansend can0 095#0300
leds-pink: cansend can0 096#
leds-100 = "cansend can0 091#8000";
leds-75 = "cansend can0 091#6000";
leds-50 = "cansend can0 091#4000";
leds-off: cansend can0 091#0000

=========================

# Zero Pivots
Type 'zero-pivots' and follow the prompts. Use 'list-blcmds' if you're looking for the ID of a specific BLCMD.

# Zero Arm
Type 'zero-arm' and follow the prompts.

=========================

# NixOS
nix-enable: sudo systemctl enable nix-daemon.service
nix-start: sudo systemctl start nix-daemon.service

=========================
