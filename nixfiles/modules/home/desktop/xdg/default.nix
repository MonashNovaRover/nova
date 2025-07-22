{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    xdg = {
      portal = {
        xdgOpenUsePortal = true;
        enable = true;
        extraPortals = [
          pkgs.xdg-desktop-portal-gnome
          pkgs.xdg-desktop-portal-gtk
        ];
        config = {
          common.default = [
            "gnome"
            "gtk"
          ];
          gnome.default = [
            "gnome"
            "gtk"
          ];
        };
      };
    };
  };
}
