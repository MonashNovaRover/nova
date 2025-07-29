{ pkgs }:

with pkgs;

{
  nova-generic-interfaces = callPackage ./generic_interfaces { };
  nova-python-control = callPackage ./python_control { };
  nova-python-control-old = callPackage ./python_control_old { };
}
