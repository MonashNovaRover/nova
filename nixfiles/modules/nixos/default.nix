{ pkgs, lib, ... }:

{
  imports = [
    ../common
    <home-manager/nixos>
    ./common
    ./profiles
  ];

  home-manager.nova.sharedModules = [ ../home ];
}
