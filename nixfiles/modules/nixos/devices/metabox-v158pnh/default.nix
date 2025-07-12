{ config, lib, ... }:

let
  cfg = config.devices.metabox-v158pnh;
in
{
  imports = [
    ./apps
    ./boot
    ./graphics
    ./oem
  ];

  options.devices.metabox-v158pnh.enable = lib.mkEnableOption "configuration for the Metabox V158PNH";

  config = lib.mkIf cfg.enable {
    nixpkgs.stdenv.hostPlatform = "x86_64-linux";
  };
}
