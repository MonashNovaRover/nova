{ config, lib, ... }:

let
  cfg = config.devices.jetson.devkit;
in
{
  options.devices.jetson.devkit.enable = lib.mkEnableOption "configuration for NVIDIA Jetson Developer Kits";

  config = lib.mkIf cfg.enable {
    hardware.nvidia-jetpack.carrierBoard = "devkit";

    # Use NetworkManager on developer kits for flexibility.
    networking.networkmanager.enable = true;

    assertions = [{
      assertion = config.devices.jetson.enable;
      message = "The Jetson Developer Kit configuration can only be enabled alongside a Jetson device configuration.";
    }];
  };
}
