# A configuration profile for shared team devices.

{ config, lib, ... }:

let
  profile = config.nova.profile;
in
{
  config = lib.mkIf (profile == "shared") {
    users.mutableUsers = false;

    services.openssh.enable = true;

    # Most fingerprint hardware and software only supports 10 fingers.
    services.fprintd.enable = false;

    nova = {
      users.nova.enable = true;
      branding.enable = true;
      desktop.enable = lib.mkDefault true;
    };

    peripherals.webcams.enable = true;
    peripherals.realsense.enable = true;
  };
}
