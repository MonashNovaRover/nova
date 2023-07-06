{ pkgs, ... }:

{
  home.packages = with pkgs.gnomeExtensions; [ dash-to-dock ];

  dconf.settings = {
    "org/gnome/shell".enabled-extensions = [ "dash-to-dock@micxgx.gmail.com" ];
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
}
