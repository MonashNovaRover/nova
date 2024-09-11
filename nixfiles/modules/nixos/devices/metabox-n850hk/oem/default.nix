{ config, lib, ... }:

{
  config = lib.mkIf config.devices.metabox-n850hk.enable {
    hardware = {
      tuxedo-keyboard.enable = true;
      tuxedo-rs = {
        enable = true;
        tailor-gui.enable = true;
      };
    };
  };
}
