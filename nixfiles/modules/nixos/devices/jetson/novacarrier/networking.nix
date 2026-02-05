{ config, lib, ... }:

{
  config = lib.mkIf config.devices.jetson.novacarrier.enable {
    nova.networking = {
      wifiIfName = "wlP1p1s0";
      ethernetIfName = "enP8p1s0";
    };
  };
}
