{ config, lib, ... }:

let
  cfg = config.devices.laptop.aftershock-light;
in
{
  options.devices.laptop.aftershock-light.enable = lib.mkEnableOption "configuration for the Aftershock Light";

  config = lib.mkIf cfg.enable {
    devices.laptop = {
      enable = true;
      intel.enable = true;
      intel-new.enable = true;
      performance.enable = true;
    };
  };
}
