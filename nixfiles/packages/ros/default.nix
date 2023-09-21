{ callPackage }:

{
  nova-workspace = callPackage ./nova-workspace { };
  rclnodejs = callPackage ./rclnodejs { };
  realsense-patches = callPackage ./realsense-patches { };
  ros-typescript-definitions = callPackage ./ros-typescript-definitions { };
}
