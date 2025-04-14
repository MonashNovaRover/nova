{ callPackage }:

{
  audio-msgs = callPackage ./audio-msgs { };
  depthai = callPackage ./depthai { };
  gst-bridge = callPackage ./gst-bridge { };
  gst-msgs = callPackage ./gst-msgs { };
  gst-pipeline = callPackage ./gst-pipeline { };
  gst-pipeline-plugins = callPackage ./gst-pipeline-plugins { };
  gst-pipeline-plugins-webrtc = callPackage ./gst-pipeline-plugins-webrtc { };
  librealsense2-gui = callPackage ./librealsense2-gui { };
  nova-workspace = callPackage ./nova-workspace { };
  ntrip-client-node = callPackage ./ntrip-client-node { };
  rclnodejs = callPackage ./rclnodejs { };
  realsense-patches = callPackage ./realsense-patches { };
  realsense-udev = callPackage ./realsense-udev { };
  ros-typescript-definitions = callPackage ./ros-typescript-definitions { };
  spatio-temporal-voxel-layer = callPackage ./spatio-temporal-voxel-layer { };
  ublox-dgnss = callPackage ./ublox-dgnss { };
  ublox-dgnss-node = callPackage ./ublox-dgnss-node { };
  ublox-nav-sat-fix-hp-node = callPackage ./ublox-nav-sat-fix-hp-node { };
  ublox-ubx-interfaces = callPackage ./ublox-ubx-interfaces { };
  ublox-ubx-msgs = callPackage ./ublox-ubx-msgs { };
  yolo-ros = callPackage ./yolo-ros { };
  yolo-msgs = callPackage ./yolo-msgs { };
  yolo-bringup = callPackage ./yolo-bringup { };
}
