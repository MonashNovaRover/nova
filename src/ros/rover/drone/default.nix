{ pkgs }:

with pkgs;

{
  nova-drone-gps = callPackage ./drone_gps { };
}
