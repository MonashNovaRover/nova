# cameras2 legacy Nixfiles

Modern versions of GStreamer (or one of its dependencies) have bugs that prevent
the camera stack from running reliably.

This directory contains package definitions for the camera stack that use an old
revision of Nixpkgs that is known to work.

The ROS distro and `nix-ros-overlay` revision is inherited from the main
codebase.

## Usage

The main way to use this old stack is through the `launcher` package. It runs
the given command with an appropriate PATH.

E.g.:

```console
$ result/bin/gst-nova-launcher ros2 launch cameras2 camera_server_launch.py
```