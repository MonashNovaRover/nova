{ config, lib, ... }:

{
  config = lib.mkIf config.devices.jetson.novacarrier.enable {

    systemd.network.links."20-can0" = {
      matchConfig = {
        Path = "platform-c310000.mttcan";
        Driver = "mttcan";
      };
      linkConfig = {
        Name = "can0";
      };
    };

    systemd.network.links."20-can1" = {
      matchConfig = {
        Path = "platform-3210000.spi-cs-0";
        Driver = "mcp251xfd";
      };
      linkConfig = {
        Name = "can1";
      };
    };

    systemd.network.links."20-can2" = {
      matchConfig = {
        Path = "platform-3230000.spi-cs-0";
        Driver = "mcp251xfd";
      };
      linkConfig = {
        Name = "can2";
      };
    };

  };
}
