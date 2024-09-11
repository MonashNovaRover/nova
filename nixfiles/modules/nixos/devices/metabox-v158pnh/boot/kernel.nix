{ config, pkgs, lib, ... }:

{
  config = lib.mkIf config.devices.metabox-v158pnh.enable {
    boot = {
      # Alder Lake support is still being improved with every kernel release.
      kernelPackages = pkgs.linuxPackages_latest;
    };

    # Allow non-free drivers.
    hardware.enableAllFirmware = true;
  };
}
