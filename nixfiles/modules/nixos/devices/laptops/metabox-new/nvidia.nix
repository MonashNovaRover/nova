{ config, lib, ... }:

let
  cfg = config.nova.laptops.metabox-new;
in
{
  # GTX 1650 Mobile
  config = lib.mkIf cfg.enable {

    hardware.nvidia = {
      prime = {
        intelBusId = "PCI:0:2:0";
        nvidiaBusId = "PCI:1:0:0";
        offload = {
          enable = false;
          enableOffloadCmd = false;
        };
        sync.enable = true; # Render everything on the NVIDIA GPU to work around offloading issues.
      };
    };

    nixpkgs.config = {
      cudaCapabilities = [ "7.5" ];
    };
  };
}
