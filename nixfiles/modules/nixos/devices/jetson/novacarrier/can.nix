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

    # TODO: get the exact ID_PATH from `udevadm info -p $(udevadm info --query=path --path=/sys/class/net/can1)`
    systemd.network.links."20-can1" = {
      matchConfig = {
        Path = "*spi0*";
        Driver = "mcp251xfd";
      };
      linkConfig = {
        Name = "can1";
      };
    };

    # TODO: get the exact ID_PATH from `udevadm info -p $(udevadm info --query=path --path=/sys/class/net/can2)`
    systemd.network.links."20-can2" = {
      matchConfig = {
        Path = "*spi1*";
        Driver = "mcp251xfd";
      };
      linkConfig = {
        Name = "can2";
      };
    };

  };
}
