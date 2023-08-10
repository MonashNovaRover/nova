{ config, lib, ... }:

{
  config = lib.mkIf config.devices.metabox-n850hk.enable {
    services.fprintd.enable = true;
  };
}
