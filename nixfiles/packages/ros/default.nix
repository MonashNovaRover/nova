{ callPackage }:

{
  depthai = callPackage ./depthai { };
  librealsense2-gui = callPackage ./librealsense2-gui { };
  nova-workspace = callPackage ./nova-workspace { };
  rclnodejs = callPackage ./rclnodejs { };
  realsense-patches = callPackage ./realsense-patches { };
  realsense-udev = callPackage ./realsense-udev { };
  ros-typescript-definitions = callPackage ./ros-typescript-definitions { };
  yolo-ros = callPackage ./yolo-ros { };
  yolo-msgs = callPackage ./yolo-msgs { };
  yolo-bringup = callPackage ./yolo-bringup { };
}
