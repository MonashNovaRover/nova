{
  config, pkgs, lib, ... }:

let
  cfg = config.peripherals.hydraprobe;
in
{
  options.peripherals.hydraprobe.enable = lib.mkEnableOption "Enable Hydraprobe USB device";

  config = lib.mkIf cfg.enable {
    services.udev.packages = [
      (pkgs.writeTextDir "lib/udev/rules.d/99-hydraprobe.rules" ''
        SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", MODE="0666"
      '')
    ];
  };
}
