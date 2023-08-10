{ config, pkgs, lib, ... }:

{
  config = lib.mkIf config.devices.metabox-v158pnh.enable {
    hardware.opengl.extraPackages = with pkgs; [ intel-media-driver ];
  };
}
