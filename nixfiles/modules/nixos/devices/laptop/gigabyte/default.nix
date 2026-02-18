{ config, lib, ... }:

let
  cfg = config.devices.laptop.gigabyte;
in
{
  imports = [
    ./nvidia.nix
  ];

  options.devices.laptop.gigabyte.enable = lib.mkEnableOption "configuration for the Gigabyte";

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
