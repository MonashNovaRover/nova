{ config, pkgs, lib, ... }:

{
  imports = [
    ../common
    <home-manager/nixos>
    ./common
    ./profiles
    ./devices
    ./peripherals
    ./secrets
  ];

  home-manager.nova.sharedModules = [
    ../home
    {
      nova = {
        inherit (config.nova) repos;
      };
    }
    {
      home.packages = with pkgs; [ sops ];
    }
  ];
}
