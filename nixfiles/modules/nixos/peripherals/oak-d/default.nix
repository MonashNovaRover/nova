{ config, pkgs, lib, ... }:

let
  cfg = config.peripherals.oakd;
in
{
  options.peripherals.oakd.enable = lib.mkEnableOption "configuration for Luxonis OAK-D peripherals";

  config = lib.mkIf cfg.enable {
    services.udev.packages = [
      (pkgs.writeTextDir "lib/udev/rules.d/80-movidius.rules" ''
        SUBSYSTEM=="usb", ATTRS{idVendor}=="03e7", MODE="0666"
      '')
    ];
  };
}
