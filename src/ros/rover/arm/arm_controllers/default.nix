{ pkgs }:

with pkgs;

{
  nova-arm-controller = callPackage ./nova_arm_controller { };
  nova-end-effector-controller = callPackage ./nova_end_effector_controller { };
  nova-twistmapper = callPackage ./nova_twistmapper { };
  nova-path-planner = callPackage ./nova_path_planner { };
  nova-banksia-kinematics-plugin = callPackage ./banksia_kinematics_plugin { };
  nova-waratah-kinematics-plugin = callPackage ./waratah_kinematics_plugin { };
}
