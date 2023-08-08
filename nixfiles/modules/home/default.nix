{ pkgs, lib, ... }:

{
  imports = [
    ../common
    ./branding
    ./desktop
    ./nova
    ./workspace
  ];
}
