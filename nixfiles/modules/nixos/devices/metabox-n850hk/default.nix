{ config, lib, ... }:

let
  cfg = config.devices.metabox-n850hk;
in
{
  imports = [
    ./boot
    ./fingerprint
    ./graphics
    ./oem
  ];

  options.devices.metabox-n850hk.enable = lib.mkEnableOption "configuration for the Metabox N850HK";

  config = lib.mkIf cfg.enable {
    nixpkgs.stdenv.hostPlatform = "x86_64-linux";
  };
}
