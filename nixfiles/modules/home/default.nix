{ pkgs, lib, ... }:

{
  imports = [
    ../common
    ./branding
    ./desktop
    ./macros
    ./monitoring
    ./nova
    ./workspace
  ];
}
