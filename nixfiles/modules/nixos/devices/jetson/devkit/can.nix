{ config, lib, ... }:

{
  config = lib.mkIf config.devices.jetson.devkit.enable {

    systemd.network.links."20-can0" = {
      matchConfig = {
        Path = "platform-c310000.mttcan";
        Driver = "mttcan";
      };
      linkConfig = {
        Name = "can0";
      };
    };

    # only present on AGX devkit
    systemd.network.links."20-can1" = {
      matchConfig = {
        Path = "platform-c320000.mttcan";
        Driver = "mttcan";
      };
      linkConfig = {
        Name = "can1";
      };
    };


  };
}
