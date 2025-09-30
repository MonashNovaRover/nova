{ lib, ... }:

let
  profiles = [
    "personal"
    "shared"
    "server"
  ];
in
{
  imports = map (profile: ./. + (/. + profile)) profiles;

  options.nova.profile = lib.mkOption {
    type = with lib.types; enum profiles;
    description = "The device configuration profile.";
    default = "personal";
  };
}
