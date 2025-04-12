{ config, pkgs, lib, ... }:

let
  cfg = config.nova.workspace;
in
{
  imports = [ ./sources.nix ];

  options.nova.workspace = {
    enable = lib.mkEnableOption "the Nova Rover ROS workspace" // { default = true; };
    package = lib.mkPackageOption pkgs [ "nova" "ros" "nova-workspace" ] { };
    gui.enable = lib.mkEnableOption "graphical applications such as RViz" // { default = false; };
  };

  config = lib.mkIf cfg.enable {
    home.packages = [
      (cfg.package.override {
        graphical = cfg.gui.enable;
      })
      pkgs.libreoffice-qt6-fresh
      pkgs.ffmpeg
    ];

    programs = {
      bash.initExtra = "eval \"$(mk-workspace-shell-setup)\"";
      zsh.initExtra = "eval \"$(mk-workspace-shell-setup)\"";
    };
  };
}
