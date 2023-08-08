{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
  extensions = with pkgs.gnomeExtensions; [
    start-overlay-in-application-view
  ];
in
{
  config = lib.mkIf cfg.enable {
    home.packages = extensions;
    dconf.settings."org/gnome/shell".enabled-extensions = map (extension: extension.extensionUuid) extensions;
  };
}
