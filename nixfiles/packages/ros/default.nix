{ callPackage }:

{
  nova-workspace = callPackage ./nova-workspace { };
  rclnodejs-message-generator = callPackage ./rclnodejs-message-generator { };
  rclnodejs-messages = callPackage ./rclnodejs-messages { };
}
