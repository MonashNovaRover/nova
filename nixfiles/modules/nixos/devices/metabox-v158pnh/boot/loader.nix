{ config, lib, ... }:

{
  config = lib.mkIf config.devices.metabox-v158pnh.enable {
    boot.loader = {
      efi.canTouchEfiVariables = true;
      systemd-boot = {
        enable = true;
        extraEntries."0.ubuntu.conf" = ''
          title Ubuntu
          efi /EFI/ubuntu/grubx64.efi
        '';
      };
      timeout = 30;
    };
  };
}
