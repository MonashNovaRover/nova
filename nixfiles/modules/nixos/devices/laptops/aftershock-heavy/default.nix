{ config, lib, ... }:

let
  cfg = config.nova.laptops.aftershock-heavy;
in
{
  imports = [
    ./nvidia.nix
  ];

  options.nova.laptops.aftershock-heavy.enable = lib.mkEnableOption "configuration for the Aftershock Heavy";

  config = lib.mkIf cfg.enable {
    nova.laptops = {
      enable = true;
      intel.enable = true;
      intel-new.enable = true;
      nvidia.enable = true;
      performance.enable = true;
    };
  };
}
