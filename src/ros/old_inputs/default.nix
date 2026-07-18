{ pkgs }:

with pkgs;

{
  nova-input-interfaces = callPackage ./input_interfaces { };
  nova-inputs = callPackage ./inputs { };
}
