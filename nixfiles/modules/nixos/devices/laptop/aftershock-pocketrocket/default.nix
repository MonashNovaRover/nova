{ config, lib, ... }:

let
  cfg = config.devices.laptop.aftershock-pocketrocket;
in
{
  imports = [
    ./nvidia.nix
  ];

  options.devices.laptop.aftershock-pocketrocket.enable = lib.mkEnableOption "configuration for the Aftershock Pocket Rocket";

  config = lib.mkIf cfg.enable {
    devices.laptop = {
      enable = true;
      intel.enable = true;
      intel-new.enable = true;
      nvidia.enable = true;
      performance.enable = true;
    };
  };
}
