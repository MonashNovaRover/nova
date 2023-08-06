{ callPackage }:

{
  nova-workspace = callPackage ./nova-workspace { };
  rclnodejs = callPackage ./rclnodejs { };
}
