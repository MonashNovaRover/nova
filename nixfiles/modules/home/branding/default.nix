{ config, pkgs, lib, ... }:

let
  cfg = config.nova.branding;
in
{
  options.nova.branding.enable = lib.mkEnableOption "Nova Rover branding";

  config = lib.mkIf cfg.enable {
    home.file.".face".source = "${pkgs.nova.nova-icons}/share/icons/hicolor/1024x1024/apps/nova-logo-white-and-orange.png";

    dconf.settings = lib.mkIf config.nova.desktop.enable {
      "org/gnome/desktop/interface" = {
        color-scheme = "prefer-dark";
      };
      "org/gnome/desktop/background" = rec {
        picture-uri = "file://${pkgs.nova.nova-backgrounds}/share/backgrounds/nova/logo-dark.png";
        picture-uri-dark = picture-uri;
        picture-options = "zoom";
      };
    };
  };
}
