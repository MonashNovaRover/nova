{ config, lib, ... }:

let
  cfg = config.devices.laptop.metabox-old;
in
{
  imports = [
    ./nvidia.nix
  ];

  options.devices.laptop.metabox-old.enable = lib.mkEnableOption "configuration for the Metabox Old";

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
