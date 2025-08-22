{ pkgs }:

with pkgs;

{
  nova-python-control = callPackage ./python_control { };
  nova-python-control2 = callPackage ./python_control2 { };
  nova-python-control-old = callPackage ./python_control_old { };
}
