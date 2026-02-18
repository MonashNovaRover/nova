{ config, lib, ... }:

let
  cfg = config.devices.laptop.aftershock-heavy;
in
{
  # RTX 5060 Mobile
  config = lib.mkIf cfg.enable {

    hardware.nvidia = {
      dynamicBoost.enable = lib.mkDefault true;
      powerManagement.enable = lib.mkDefault true;
      powerManagement.finegrained = lib.mkDefault true;
      prime = {
        intelBusId = "PCI:0:2:0";
        nvidiaBusId = "PCI:1:0:0";
        offload = {
          enable = lib.mkDefault true;
          enableOffloadCmd = lib.mkDefault true;
        };
      };
    };

    nixpkgs.config = {
      cudaSupport = lib.mkDefault true;
      cudaCapabilities = [ "12.0" ];
    };
  };
}
