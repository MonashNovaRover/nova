{ pkgs, lib, ... }:

{
  imports = [
    ../common
    ./branding
    ./desktop
    ./macros
    ./nova
    ./workspace
  ];
}
