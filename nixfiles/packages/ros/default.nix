{ callPackage }:

{
  nova-workspace = callPackage ./nova-workspace { };
  rclnodejs = callPackage ./rclnodejs { };
  ros-typescript-definitions = callPackage ./ros-typescript-definitions { };
}
