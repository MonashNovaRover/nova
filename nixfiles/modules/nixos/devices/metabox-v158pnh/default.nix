{ lib, ... }:

{
  imports = [
    ./boot
    ./graphics
    ./keyboard
  ];

  options.devices.metabox-v158pnh.enable = lib.mkEnableOption "configuration for the Metabox V158PNH";
}
