{ pkgs }:

with pkgs;

{
  nova-pivot-drive-controller = callPackage ./pivot_drive_controller { };
  nova-strafe-controller = callPackage ./strafe_controller { };
  nova-diff-drive-controller = callPackage ./nova_diff_drive_controller { };
  nova-arm-controller = callPackage ./nova_arm_controller { };
  nova-twistmapper = callPackage ./nova_twistmapper { };
  nova-path-planner = callPackage ./nova_path_planner { };
  nova-banksia-kinematics-plugin = callPackage ./banksia_kinematics_plugin { };
  nova-waratah-kinematics-plugin = callPackage ./waratah_kinematics_plugin { };
}
