{ pkgs }:

with pkgs;

{
  nova-drive-controller-base = callPackage ./drive_controller_base { };
  nova-pivot-drive-controller = callPackage ./pivot_drive_controller { };
  nova-strafe-controller = callPackage ./strafe_controller { };
  nova-diff-drive-controller = callPackage ./diff_drive_controller { };
}
