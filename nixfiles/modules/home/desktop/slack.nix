{ config, lib, pkgs, ... }:

let
  cfg = config.nova.desktop.slack;
in
{
  options.nova.desktop.slack.enable = lib.mkEnableOption "Nova Rover user Slack configuration";

  config = lib.mkIf cfg.enable {
    home.packages = with pkgs; [
      slack
    ];

    dconf.settings."org/gnome/shell".favorite-apps = lib.mkAfter [
      "slack.desktop"
    ];
  };
}
