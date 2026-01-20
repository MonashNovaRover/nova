{ pkgs }:

with pkgs;

{
  nova-excavation-construction = callPackage ./excavation_construction { };
  nova-teleop-ec = callPackage ./teleop_ec { };
}