{ pkgs }:

with pkgs;

{
  #nova-four-wheel-steering-controller = callPackage ./four-wheel-steering-controller { };
  nova-pivot-drive-controller = callPackage ./pivot-drive-controller { };
  #nova-four-steering-controller = callPackage ./four-steering-controller { };
}
