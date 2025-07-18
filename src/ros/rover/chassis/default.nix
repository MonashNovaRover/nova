{ pkgs }:

with pkgs;

{
  nova-electronics = callPackage ./nix/packages/electronics { };
}
