{ config, pkgs, lib, ... }:

let
  cfg = config.nova.branding;
in
{
  options.nova.branding.enable = lib.mkEnableOption "Nova Rover branding";

  config = lib.mkIf cfg.enable {
    system.nixos.tags = [ "nova" ];

    boot = {
      plymouth = {
        enable = true;
        logo = "${pkgs.nova.nova-icons}/share/icons/hicolor/128x128/apps/nova-logo-white.png";
      };
    };

    programs.dconf.profiles.gdm.databases = lib.mkIf config.services.displayManager.gdm.enable [{
      settings = {
        "org/gnome/login-screen".logo = "${pkgs.nova.nova-icons}/share/icons/hicolor/512x512/apps/nova-logo-white.png";
      };
    }];

    home-manager.nova.sharedModules = [{
      nova.branding.enable = true;
    }];
  };
}
