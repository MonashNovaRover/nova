{ config, lib, ... }:

let
  cfg = config.nova.desktop.slack;
in
{
  options.nova.desktop.slack.enable = lib.mkEnableOption "Nova Rover user Slack configuration";
  
  config = lib.mkIf cfg.enable {
    home-manager.nova.sharedModules = [{ nova.desktop.slack.enable = true; }];
  };
}
