{ pkgs }:

with pkgs;

{
  nova-pivot-drive-controller = callPackage ./pivot-drive-controller { };
  nova-strafe-controller = callPackage ./strafe-controller { };

}
