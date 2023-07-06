{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  options.nova.desktop = {
    enable = lib.mkEnableOption "the standard Nova Rover desktop environment";
    wayland.enable = lib.mkEnableOption "Wayland" // { default = true; };
  };

  config = lib.mkIf cfg.enable {
    services.xserver = {
      displayManager.gdm = {
        enable = true;
        wayland = cfg.wayland.enable;
      };
      desktopManager.gnome.enable = true;
      excludePackages = with pkgs; [
        xterm # Use the desktop environment's terminal instead.
      ];
    };

    services.gnome = {
      # Unwanted general computing services
      # (search indexing, online services, games, etc.)
      gnome-initial-setup.enable = false;
      gnome-user-share.enable = false;
      gnome-remote-desktop.enable = false;
      rygel.enable = false;
      tracker.enable = false;
      tracker-miners.enable = false;
      # gnome-online-miners.enable = false; # Set strongly for some reason
      gnome-online-accounts.enable = false;
      # evolution-data-server.enable = false; # Set strongly for some reason
      games.enable = false;

      # Unwanted GNOME services
      gnome-browser-connector.enable = false; # Extensions should be added by Nix

      # Useful GNOME services
      sushi.enable = true;
    };

    environment.gnome.excludePackages = with pkgs.gnome; [
      # Unneeded GNOME shell components
      # https://gitlab.gnome.org/GNOME/gnome-build-meta/blob/gnome-3-38/elements/core/meta-gnome-core-shell.bst
      gnome-backgrounds # We set custom backgrounds
      gnome-color-manager # We should configure colour profiles in Nix
      pkgs.gnome-tour # We know how to use GNOME

      # Unneeded GNOME core utilities
      # https://gitlab.gnome.org/GNOME/gnome-build-meta/blob/gnome-3-38/elements/core/meta-gnome-core-utilities.bst
      baobab
      epiphany
      gnome-boxes
      gnome-calendar
      gnome-contacts
      gnome-maps
      gnome-music
      pkgs.gnome-photos
      pkgs.gnome-connections
      gnome-weather
      simple-scan
      totem

      # Unneeded GNOME applications and services enabled by NixOS
      geary
    ];

    environment.systemPackages = with pkgs; [
      clapper
    ];

    home-manager.nova.sharedModules = [{
      nova.desktop.enable = true;
      nova.desktop.wayland.enable = cfg.wayland.enable;
    }];

    warnings = lib.optional (!config.services.xserver.enable) ''
      The Nova Rover desktop environment has been enabled, but the display server has not.
      Set services.xserver.enable to true to enable the display server.
    '';
  };
}
