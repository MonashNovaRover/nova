{ config, lib, ... }:

let
  cfg = config.devices.jetson.orin-nano;
in
{
  imports = [
    ./devkit
  ];

  options.devices.jetson.orin-nano.enable = lib.mkEnableOption "configuration for the NVIDIA Jetson Orin Nano" // { internal = true; };

  config = lib.mkIf cfg.enable {
    devices.jetson.enable = true;
    hardware.nvidia-jetpack.som = "orin-nano";
  };
}
