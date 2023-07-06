{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  imports = [
    ./extensions
  ];

  options.nova.desktop = {
    enable = lib.mkEnableOption "the standard Nova Rover desktop apps and configuration";
    wayland.enable = lib.mkEnableOption "Wayland" // { default = true; };
  };

  config = lib.mkIf cfg.enable {
    dconf.settings = {
      # GNOME desktop settings schemas can be found here:
      # https://gitlab.gnome.org/GNOME/gsettings-desktop-schemas/-/tree/master/schemas

      "org/gnome/mutter" = {
        attach-modal-dialogs = true; # Can reduce confusion due to misplaced popups

        # Multitasking
        edge-tiling = true;
        dynamic-workspaces = true;
      };

      "org/gnome/shell" = {
        # Extension customization
        disable-user-extensions = false;
        disabled-extensions = [ ];
        enabled-extensions =
          # Useful official extensions (https://gitlab.gnome.org/GNOME/gnome-shell-extensions)
          map (name: "${name}@gnome-shell-extensions.gcampax.github.com") [
            "apps-menu"
            "places-menu"
            "drive-menu"
            "native-window-placement"
            "workspace-indicator"
          ];
      };

      "org/gnome/desktop/session" = {
        idle-delay = 15 * 60; # 15 minutes before screen off
      };

      "org/gnome/desktop/interface" = {
        # Clock settings
        clock-show-weekday = true; # Why not? Otherwise wasted space.
        clock-show-seconds = true; # Useful during competition

        # Accesibility improvements for multi-monitor usage
        enable-hot-corners = false; # Too easily triggered when moving between monitors
        locate-pointer = true; # The pointer can often be hard to find
      };

      "org/gnome/desktop/sound" = {
        allow-volume-above-100-percent = true;
      };
    };
  };
}
