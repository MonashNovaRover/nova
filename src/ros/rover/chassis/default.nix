{ pkgs }:

with pkgs;

{
  nova-electronics = callPackage ./electronics { };
}
