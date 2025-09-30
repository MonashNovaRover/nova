{ config, lib, ... }:

let
  cfg = config.devices.jetson.orin-nano;
in
{
  imports = [
    ./boot
  ];

  options.devices.jetson.orin-nano.enable = lib.mkEnableOption "configuration for the NVIDIA Jetson Orin Nano";

  config = lib.mkIf cfg.enable {
    devices.jetson.enable = true;
    hardware.nvidia-jetpack.som = "orin-nano";

    hardware.xone.enable = lib.mkForce false;

    # Display output is non-functional on the Orin Nano.
    # https://github.com/anduril/jetpack-nixos/issues/85
    services.xserver.enable = false;
    nova.desktop.enable = false;
  };
}
