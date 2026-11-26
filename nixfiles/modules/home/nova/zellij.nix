{ config, lib, ... }:

let
  cfg = config.nova.user;
in
{
  config = lib.mkIf cfg.enable {
    programs.zellij = {
      enable = true;
      settings = {
        theme_dark = "catppuccin-mocha";
        theme_light = "catppuccin-latte";
        show_startup_tips = false;
      };
    };
  };
}
