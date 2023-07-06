{ config, pkgs, lib, ... }:

let
  cfg = config.nova.workspace;
in
{
  options.nova.workspace = {
    enable = lib.mkEnableOption "the Nova Rover ROS workspace" // { default = true; };
    gui.enable = lib.mkEnableOption "graphical applications such as RViz" // { default = false; };
  };

  config = lib.mkIf cfg.enable {
    home.packages = [
      (pkgs.nova.ros.nova-workspace.override {
        includeGraphicalApplications = cfg.gui.enable;
      })
    ];

    programs = {
      bash.initExtra = "eval \"$(mk-nova-shell-setup)\"";
      zsh.initExtra = "eval \"$(mk-nova-shell-setup)\"";
    };
  };
}
