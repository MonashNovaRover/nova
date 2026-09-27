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
      gnome-terminal = {
        enable = true;
        profile.c661e430-2d09-4470-993d-d45e65eb4f84 = {
          visibleName = "";
          default = true;
        };
      };
    };

    # Network bandwidth by process/application monitor
    # On NixOS, nethogs is provided via a security wrapper with elevated capabilities instead
    home.packages = with pkgs; lib.optionals (!onNixOS) [
      nethogs
    ];
  };
}
