{ config, pkgs, lib, ... }:

let
  cfg = config.peripherals.realsense;
in
{
  options.peripherals.realsense.enable = lib.mkEnableOption "configuration for Intel RealSense peripherals";

  config = lib.mkIf cfg.enable {
    hardware.enableRedistributableFirmware = true;
    services.udev.packages = [ pkgs.librealsense ];
  };
}
