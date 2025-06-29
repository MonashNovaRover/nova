{ pkgs }:

with pkgs;

{
  nova-generic-can-nodes = callPackage ./generic-can-nodes { };
  nova-generic-interfaces = callPackage ./generic-interfaces { };
}
