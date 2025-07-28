{ pkgs }:

with pkgs;

{
  nova-generic-can-nodes = callPackage ./generic_can_nodes { };
  nova-generic-interfaces = callPackage ./generic_interfaces { };
}
