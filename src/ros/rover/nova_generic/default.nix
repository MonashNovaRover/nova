{ pkgs }:

with pkgs;

{
  nova-python-control = callPackage ./python_control { };
  nova-python-control-old = callPackage ./python_control_old { };
}
