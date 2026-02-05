{ config, lib, ... }:

let
  cfg = config.devices.jetson.novacarrier;
in
{
  options.devices.jetson.novacarrier.enable = lib.mkEnableOption "configuration for Monash Nova Rover NVIDIA Jetson Orin Nano Carrier Board";

  imports = [
    ./can.nix
    ./networking.nix
  ];

  config = lib.mkIf cfg.enable {
    # TODO: this isn't true, but it doesn't matter as long as you don't try to start a firmware update
    hardware.nvidia-jetpack.carrierBoard = "devkit";

    # Use NetworkManager for flexibility.
    networking.networkmanager.enable = true;

    assertions = [
      {
        assertion = config.devices.jetson.enable;
        message = "The novacarrier configuration can only be enabled alongside a Jetson device configuration.";
      }
      {
        assertion = config.devices.jetson.orin-nano.enable;
        message = "The novacarrier configuration can only be enabled alongside a Orin Nano device configuration.";
      }
    ];
  };
}
