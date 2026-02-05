{ config, lib, ... }:

{
  config = lib.mkIf config.devices.jetson.devkit.enable {
    nova.networking = {
      wifiIfName = "wlP1p1s0";
      ethernetIfName = lib.mkIf config.devices.orin-nano.enable "enP8p1s0";
      ethernetIfName = lib.mkIf config.devices.orin-agx.enable "eth0";
    };
  };
}
