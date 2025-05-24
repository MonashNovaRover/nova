{ pkgs }:

with pkgs;

{
  nova-pivot-drive-controller = callPackage ./pivot-drive-controller { };
  nova-strafe-controller = callPackage ./strafe-controller { };
  nova-diff-drive-controller = callPackage ./nova-diff-drive-controller { };
  nova-arm-controller = callPackage ./nova-arm-controller { };
  nova-twistmapper = callPackage ./nova-twistmapper { };
  nova-path-planner = callPackage ./nova-path-planner { };
  nova-banksia-kinematics-plugin = callPackage ./nova-banksia-kinematics-plugin { };
  nova-waratah-kinematics-plugin = callPackage ./nova-waratah-kinematics-plugin { };
}
