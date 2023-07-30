{ pkgs, ... }:

let
  extensions = with pkgs.gnomeExtensions; [
    start-overlay-in-application-view
  ];
in
{
  home.packages = extensions;
  dconf.settings."org/gnome/shell".enabled-extensions = map (extension: extension.extensionUuid) extensions;
}
