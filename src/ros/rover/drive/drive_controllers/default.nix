{ pkgs }:

with pkgs;

{
  nova-drive-controller-base = callPackage ./nova_drive_controller_base { };
  nova-pivot-drive-controller = callPackage ./pivot_drive_controller { };
  nova-strafe-drive-controller = callPackage ./strafe_drive_controller { };
  nova-diff-drive-controller = callPackage ./diff_drive_controller { };
  # Override the upstream diff-drive-controller so only one version ends up in the workspace.
  diff-drive-controller = callPackage ./diff_drive_controller { };
}
