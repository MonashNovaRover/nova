{ config, lib, ... }:

let
  cfg = config.devices.raspberry-pi;
in
{
  imports = [
    ./devices
  ];

  options.devices.raspberry-pi.enable = lib.mkEnableOption "configuration for the raspberry-pi";

  config = lib.mkIf cfg.enable {
    nixpkgs.hostPlatform = "aarch64-linux";
  };
}
