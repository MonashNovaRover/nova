{ config, lib, ... }:

{
  config = lib.mkIf config.devices.metabox-n850hk.enable {
    boot.extraModulePackages = with config.boot.kernelPackages; [ tuxedo-keyboard ];
  };
}
