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
  livox-ros-driver2 = callPackage ./livox-ros-driver2 { };
  livox-sdk2 = callPackage ./livox-sdk2 { };
  nova-workspace = callPackage ./nova-workspace { };
  r3live = callPackage ./r3live { };
  rclnodejs = callPackage ./rclnodejs { };
  realsense-udev = callPackage ./realsense-udev { };
  ros-typescript-definitions = callPackage ./ros-typescript-definitions { };
  spatio-temporal-voxel-layer = callPackage ./spatio-temporal-voxel-layer { };
  yolo-ros = callPackage ./yolo-ros { };
  yolo-msgs = callPackage ./yolo-msgs { };
  yolo-bringup = callPackage ./yolo-bringup { };
}
