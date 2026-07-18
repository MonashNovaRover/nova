{ pkgs }:

with pkgs;

{
  nova-dgnss = callPackage ./dgnss { };
  nova-electronics = callPackage ./electronics { };
}
