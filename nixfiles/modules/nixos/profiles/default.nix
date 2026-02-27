{ lib, ... }:

let
  profiles = [
    "personal"
    "shared"
    "server"
    "mast"
  ];
in
{
  imports = map (profile: ./. + (/. + profile)) profiles ++ [ ./common ];

  options.nova.profile = lib.mkOption {
    type = with lib.types; enum profiles;
    description = "The device configuration profile.";
    default = "personal";
  };
}
