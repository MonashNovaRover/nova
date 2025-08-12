{ pkgs }:

with pkgs;

{
  nova-joint-space-control-mode = callPackage ./joint_space_control_mode { };
}
