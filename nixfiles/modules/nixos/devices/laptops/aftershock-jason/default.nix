{ config, lib, ... }:

let
  cfg = config.nova.laptops.aftershock-jason;
in
{
  imports = [
    ./nvidia.nix
  ];

  options.nova.laptops.aftershock-jason.enable = lib.mkEnableOption "configuration for the Aftershock Jason";

  config = lib.mkIf cfg.enable {
    nova.laptops = {
      enable = true;
      intel.enable = true;
      intel-old.enable = true;
      nvidia.enable = true;
      performance.enable = true;
    };
  };
}
