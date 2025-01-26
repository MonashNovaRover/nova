{ pkgs }:

with pkgs;

{
  nova-pivot-drive-controller = callPackage ./pivot-drive-controller { };
  nova-strafe-controller = callPackage ./strafe-controller { };
  nova-diff-drive-controller = callPackage ./nova-diff-drive-controller { };
  nova-arm-controller = callPackage ./nova-arm-controller { };
}
