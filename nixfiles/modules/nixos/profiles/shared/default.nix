# A configuration profile for shared team devices.

{ config, lib, ... }:

let
  profile = config.nova.profile;
in
{
  config = lib.mkIf (profile == "shared") {
    boot.kernel.sysctl = {
      "kernel.dmesg_restrict" = false;
    };

    services.openssh = {
      enable = true;
      settings.X11Forwarding = true;
    };

    # Most fingerprint hardware and software only supports 10 fingers.
    services.fprintd.enable = false;

    users.mutableUsers = false;

    nova = {
      users.nova.enable = true;
      branding.enable = true;
      desktop.enable = lib.mkDefault true;
    };

    peripherals.webcams.enable = true;
    peripherals.realsense.enable = true;
  };
}
