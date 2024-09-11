{ config, pkgs, lib, ... }:

{
  config = lib.mkIf config.devices.metabox-n850hk.enable {
    # Allow non-free drivers.
    hardware.enableAllFirmware = true;
  };
}
