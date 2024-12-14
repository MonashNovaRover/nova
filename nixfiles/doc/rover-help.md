# Running the Rover
=========================

## Connecting
Type 'jetson' to SSH into the rover (via ethernet or base station). If you're on the makerspace wifi, you'll need to use one of the following commands.
J1: ssh nvidia@10.0.2.21
J2: ssh nvidia@10.0.2.22
J3: ssh nvidia@10.0.2.23

=========================

## Base
Launch this on the metabox. Try the 'launch-base' alias, otherwise use the command below.
~/Builds/master/bin/ros2 launch nova_bringup base.launch.py

## Drive
Launch this on the jetson. Try the 'launch-drive' alias, otherwise use the command below.
~/Builds/master/bin/ros2 launch nova_bringup drive.launch.py

=========================

## Arm

## C&E
If using the construction & excavation payload, you DON'T need to run drive, only base.
~/Builds/scraper/bin/ros2 launch nova_bringup ec_rover.launch.py

## Science

=========================

## Cameras
Replace '?' with either 'arm', 'ce', or 'science'.
cameras_all payload:=?


