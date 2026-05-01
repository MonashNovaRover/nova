{ pkgs }:

with pkgs;

{
  nova-banksia-kinematics-plugin = callPackage ./banksia_kinematics_plugin { };
  nova-waratah-kinematics-plugin = callPackage ./waratah_kinematics_plugin { };
}
