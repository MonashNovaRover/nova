# A configuration profile for shared team devices.

{ config, lib, ... }:

let
  profile = config.nova.profile;
in
{
  config = lib.mkIf (profile == "shared") {
    users.mutableUsers = false;

    services.openssh.enable = true;

    nova = {
      users.nova.enable = true;
      branding.enable = true;
      desktop.enable = lib.mkDefault true;
    };
  };
}
