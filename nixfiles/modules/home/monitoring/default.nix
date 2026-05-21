{ config, lib, pkgs, osConfig ? null, ... }:

let
  cfg = config.nova.monitoring;
  onNixOS = osConfig != null;
in
{
  options.nova.monitoring.enable = lib.mkEnableOption "Install monitoring tools";

  config = lib.mkIf cfg.enable {
    # General resource monitor
    programs = {
      btop.enable = true;
    };

    # Network bandwidth by process/application monitor
    # On NixOS, nethogs is provided via a security wrapper with elevated capabilities instead
    home.packages = with pkgs; lib.optionals (!onNixOS) [
      nethogs
    ];
  };
}
