{ config, lib, ... }:

let
  cfg = config.nova.laptops.aftershock-pocketrocket;
in
{
  imports = [
    ./nvidia.nix
  ];

  options.nova.laptops.aftershock-pocketrocket.enable = lib.mkEnableOption "configuration for the Aftershock Pocket Rocket";

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
