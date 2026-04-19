{ pkgs }:

with pkgs;

{
  nova-arm-controller-old = callPackage ./nova_arm_controller_old { };
  nova-twistmapper-old = callPackage ./nova_twistmapper_old { };
  nova-path-planner-old = callPackage ./nova_path_planner_old { };
  nova-banksia-kinematics-plugin = callPackage ./banksia_kinematics_plugin { };
  nova-waratah-kinematics-plugin = callPackage ./waratah_kinematics_plugin { };
}
