{ config, lib, ... }:

{
  config = lib.mkIf config.devices.metabox-v158pnh.enable {
    hardware = {
      tuxedo-keyboard.enable = true;
      tuxedo-rs = {
        enable = true;
        tailor-gui.enable = true;
      };
    };
  };
}
