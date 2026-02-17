{ config, lib, ... }:

let
  cfg = config.nova.laptops.aftershock-heavy;
in
{
  # RTX 5060 Mobile
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
      cudaCapabilities = [ "12.0" ];
    };
  };
}
