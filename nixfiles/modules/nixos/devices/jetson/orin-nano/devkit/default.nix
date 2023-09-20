{ config, lib, ... }:

let
  cfg = config.devices.jetson.orin-nano.devkit;
in
{
  options.devices.jetson.orin-nano.devkit.enable = lib.mkEnableOption "configuration for the NVIDIA Jetson Orin Nano Developer Kit";

  config = lib.mkIf cfg.enable {
    devices.jetson.orin-nano.enable = true;
    hardware.nvidia-jetpack.carrierBoard = "devkit";
  };
}
