{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    home.packages = with pkgs; [ blackbox-terminal ghostty ptyxis ];

    programs.ssh.extraConfig = ''
      Host *
        SetEnv TERM=xterm-256color
    '';
  };
}
