{ lib, ... }:

{
  imports = [
    ./boot
    ./graphics
    ./keyboard
  ];

  options.devices.metabox-n850hk.enable = lib.mkEnableOption "configuration for the Metabox N850HK";
}
