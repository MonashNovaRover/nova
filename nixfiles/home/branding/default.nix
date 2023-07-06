{ config, pkgs, lib, ... }:

let
  cfg = config.nova.branding;
in
{
  options.nova.branding.enable = lib.mkEnableOption "Nova Rover branding";

  config = lib.mkIf cfg.enable {
    home.file.".face".source = "${pkgs.nova.nova-icons}/share/icons/hicolor/1024x1024/apps/nova-logo-white-and-orange.png";

    dconf.settings = {
      "org/gnome/desktop/interface" = {
        color-scheme = "prefer-dark";
      };
      "org/gnome/desktop/background" = rec {
        picture-uri = "file://${pkgs.runCommand "nova-desktop-background.png" { nativeBuildInputs = [ pkgs.imagemagick ]; } ''
        convert \
          -resize 820x820 \
          -gravity center \
          -background '#434343' \
          -extent 3840x2160 \
          ${pkgs.nova.nova-icons}/share/icons/hicolor/1024x1024/apps/nova-logo-white-and-orange.png \
          "$out"
      ''}";
        picture-uri-dark = picture-uri;
        picture-options = "zoom";
      };
    };
  };
}
