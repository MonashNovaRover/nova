{ config, lib, ... }:

let
  cfg = config.devices.laptop.gigabyte;
in
{
  # RTX 4060 Mobile
  config = lib.mkIf cfg.enable { 

    hardware.nvidia = {
      prime = {
        intelBusId = "PCI:0:2:0";
        nvidiaBusId = "PCI:1:0:0";
        offload = {
          enable = false;
          enableOffloadCmd = false;
        };
      };
    };

    nixpkgs.config = {
      cudaCapabilities = [ "8.9" ];
    };
  };
}
