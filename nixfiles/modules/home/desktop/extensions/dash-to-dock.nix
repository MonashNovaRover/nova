{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
  extensions = with pkgs.gnomeExtensions; [ dash-to-dock ];
in
{
  config = lib.mkIf cfg.enable {
    home.packages = extensions;
    dconf.settings = {
      "org/gnome/shell".enabled-extensions = map (extension: extension.extensionUuid) extensions;
      "org/gnome/shell/extensions/dash-to-dock" = {
        # Multi-monitor
        multi-monitor = true;
        isolate-monitors = true;

        # Layout
        dock-position = "LEFT";
        dock-fixed = true;
        extend-height = true;
        icon-size-fixed = true;

        # Appearance
        disable-overview-on-startup = true;
        running-indicator-style = "DASHES";
        custom-background-color = true;
        background-color = "rgb(0,0,0)";
        transparency-mode = "FIXED";
        background-opacity = 0.75;
        show-trash = false;
      };
    };
  };
}
