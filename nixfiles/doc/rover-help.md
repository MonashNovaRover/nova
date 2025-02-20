
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
             
             Jetson Radios: ssh nvidia@10.0.0.10
             Orin Radios: ssh nova@10.0.0.11

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

# Arm
Launch this on the jetson.
When running the arm payload, you DO need to run drive.
Try the 'launch-arm' alias, otherwise use the command below.
~/Builds/master/bin/ros2 launch nova_bringup arm.launch.py

# C&E
Launch this on the jetson.
When running the construction & excavation payload, you DON'T need to run drive.
Try the 'launch-ec' alias, otherwise use the command below.
~/Builds/master/bin/ros2 launch nova_bringup ec_rover.launch.py

# Science

=========================

# Cameras
Replace '?' with either 'arm', 'ce', or 'science'.
cameras-legacy payload:=?

=========================

# Zero Pivots
Type 'zero-pivots' and follow the prompts. Use 'list-blcmds' if you're looking for the ID of a specific BLCMD.

# Zero Arm
Type 'zero-arm' and follow the prompts.

=========================
