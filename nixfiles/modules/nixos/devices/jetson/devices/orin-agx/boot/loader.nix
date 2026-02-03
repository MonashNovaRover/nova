{ config, lib, ... }:

{
  config = lib.mkIf config.devices.jetson.orin-agx.enable {
    boot.loader = {
      efi.canTouchEfiVariables = true;
      systemd-boot.enable = true;
    };
  };
}
