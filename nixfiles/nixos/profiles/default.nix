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
    description = lib.mdDoc "The device configuration profile.";
  };
}
