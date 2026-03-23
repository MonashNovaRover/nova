{ config, pkgs, lib, ... }:
let
  cfg = config.devices.jetson;
in
{
  config = lib.mkIf cfg.enable {
    boot.kernelPatches = [
      {
        name = "net drivers";
        patch = null;
        extraConfig = ''
          HSR m
          VXLAN m
        '';
      }
    ];
  };
}

