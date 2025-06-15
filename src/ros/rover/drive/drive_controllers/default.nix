{ pkgs }:

with pkgs;

{
  nova-controller-common = callPackage ./nova_controller_common { };
  nova-pivot-drive-controller = callPackage ./pivot_drive_controller { };
  nova-strafe-controller = callPackage ./strafe_controller { };
  nova-diff-drive-controller = callPackage ./nova_diff_drive_controller { };
}
