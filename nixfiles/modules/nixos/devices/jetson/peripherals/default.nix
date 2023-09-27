{ config, pkgs, lib, ... }:

let
  cfg = config.devices.jetson;
in
{
  config = lib.mkIf cfg.enable { };
}
