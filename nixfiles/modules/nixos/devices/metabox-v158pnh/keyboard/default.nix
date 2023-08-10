{ config, lib, ... }:

{
  config = lib.mkIf config.devices.metabox-v158pnh.enable {
    boot.extraModulePackages = with config.boot.kernelPackages; [ tuxedo-keyboard ];
  };
}
