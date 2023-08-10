{ config, pkgs, lib, ... }:

{
  imports = [
    ../common
    <home-manager/nixos>
    ./common
    ./profiles
    ./devices
  ];

  home-manager.nova.sharedModules = [
    ../home
    {
      nova = {
        inherit (config.nova) repos;
      };
    }
  ];
}
