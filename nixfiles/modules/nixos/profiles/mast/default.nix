# A configuration profile for devices serving the role of the radio mast (for dgps)

{ config, lib, pkgs, ... }:

let
  profile = config.nova.profile;
in
{
  config = lib.mkIf (profile == "mast") {
    nova.networking.mast.enable = true;

    environment.systemPackages = with pkgs; [
      nova.ros.nova-workspace-mast
    ];

    peripherals.gps.enable = true;
  };
}
