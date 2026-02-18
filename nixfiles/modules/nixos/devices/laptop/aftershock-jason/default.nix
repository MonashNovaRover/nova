{ config, lib, ... }:

let
  cfg = config.devices.laptop.aftershock-jason;
in
{
  imports = [
    ./nvidia.nix
  ];

  options.devices.laptop.aftershock-jason.enable = lib.mkEnableOption "configuration for the Aftershock Jason";

  config = lib.mkIf cfg.enable {
    devices.laptop = {
      enable = true;
      intel.enable = true;
      intel-old.enable = true;
      nvidia.enable = true;
      performance.enable = true;
    };
  };
}
