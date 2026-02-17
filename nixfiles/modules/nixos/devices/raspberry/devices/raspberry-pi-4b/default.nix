{ config, lib, ... }:

let
  cfg = config.devices.raspberry-pi.rp4b;
in
{
  imports = [
    ./boot
  ];

  options.devices.raspberry-pi.rp4b.enable = lib.mkEnableOption "configuration for the Raspberry Pi 4B";

  config = lib.mkIf cfg.enable {
    devices.raspberry-pi.enable = true;

    nova.networking.wifiInterface = "wlan0";
    nova.networking.ethernetInterface = "end0";
  };
}
