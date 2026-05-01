{ pkgs }:

with pkgs;

{
  nova-arm-kinematics = callPackage ./arm_kinematics { };
  nova-arm-kinematics-benchmark = callPackage ./benchmark { };
}

