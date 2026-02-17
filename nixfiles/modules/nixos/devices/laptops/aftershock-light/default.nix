{ config, lib, ... }:

let
  cfg = config.nova.laptops.aftershock-light;
in
{
  options.nova.laptops.aftershock-light.enable = lib.mkEnableOption "configuration for the Aftershock Light";

  config = lib.mkIf cfg.enable {
    nova.laptops = {
      enable = true;
      intel.enable = true;
      intel-new.enable = true;
      performance.enable = true;
    };
  };
}
