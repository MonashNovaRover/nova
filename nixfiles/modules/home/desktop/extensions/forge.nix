{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
  extensions = with pkgs.gnomeExtensions; [ forge ];
in
{
  config = lib.mkIf cfg.enable {
    home.packages = extensions;
    dconf.settings = {
      "org/gnome/shell".enabled-extensions = map (extension: extension.extensionUuid) extensions;
      "org/gnome/shell/extensions/forge" = {
      };
    };
  };
}
