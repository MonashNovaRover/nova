{
  config, pkgs, lib, ... }:

let
  cfg = config.peripherals.xone0_4_12;
  xone = config.boot.kernelPackages.callPackage ./xone.nix {};
in
{
  options.peripherals.xone0_4_12.enable = lib.mkEnableOption "Enable xone driver version 0.4.12, the last version supported by linux 5.15";

  config = lib.mkIf cfg.enable {
    hardware.firmware = [ pkgs.xow_dongle-firmware ];
    boot = {
      blacklistedKernelModules = [
        "xpad"
        "mt76x2u"
      ];
      extraModulePackages = with config.boot.kernelPackages; [
        xone
      ];
    };

    # disable upstream version
    hardware.xone.enable = lib.mkForce false;
  };
}
