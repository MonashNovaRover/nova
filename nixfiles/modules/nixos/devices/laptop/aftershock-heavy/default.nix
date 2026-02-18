{ config, lib, ... }:

let
  cfg = config.devices.laptop.aftershock-heavy;
in
{
  imports = [
    ./nvidia.nix
  ];

  options.devices.laptop.aftershock-heavy.enable = lib.mkEnableOption "configuration for the Aftershock Heavy";

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
