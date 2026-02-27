{
  config, pkgs, lib, ... }:

let
  cfg = config.peripherals.gps;
in
{
  options.peripherals.gps.enable = lib.mkEnableOption "Enable GPS USB devices";

  config = lib.mkIf cfg.enable {
    services.udev.packages = [
      (pkgs.writeTextDir "lib/udev/rules.d/99-ublox-gnss.rules" ''
        ATTRS{idVendor}=="1546", ATTRS{idProduct}=="01a9", MODE="0666", GROUP="plugdev"
      '')
    ];
  };
}
