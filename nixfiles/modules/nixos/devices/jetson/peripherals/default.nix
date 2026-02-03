{ config, pkgs, lib, ... }:

let
  cfg = config.devices.jetson;
in
{
  config = lib.mkIf cfg.enable {
    # kernel 5.15 can't go any higher than this version
    peripherals.xone0_4_12.enable = true;
  };
}
