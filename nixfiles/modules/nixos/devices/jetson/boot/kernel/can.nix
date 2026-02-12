{ config, pkgs, lib, ... }:
let
  cfg = config.devices.jetson;
in
{
  config = lib.mkIf cfg.enable {
    boot.kernelPatches = [
      # https://github.com/MonashNovaRover/novacarrier-assets/blob/r36.4.3/flash/spi-tegra114.txt
      {
        name = "mcp251xfd-bug-patch";
        patch = ./patches/0001-mcp251xfd-bug-patch.patch;
      }
      {
        name = "enable can drivers";
        patch = null;
        extraConfig = ''
          CAN_MCP251XFD m
          CAN_GS_USB m
        '';
      }
    ];
  };
}

