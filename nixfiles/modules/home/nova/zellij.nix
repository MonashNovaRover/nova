{ config, lib, ... }:

let
  cfg = config.nova.user;
in
{
  config = lib.mkIf cfg.enable {
    programs.zellij = {
      enable = true;
      settings = {
        theme = "catppuccin-frappe";
        show_startup_tips = false;
      };
    };
  };
}
