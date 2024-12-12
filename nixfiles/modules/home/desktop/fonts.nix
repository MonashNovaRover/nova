{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    fonts = {
      fontconfig = {
        enable = true;

        defaultFonts = {
          # nerdfonts don't display without this definition
          monospace = [ "Monospace 12" ];
        };
      };
    };

    home.packages = with pkgs; [
      nerd-fonts._0xproto
      nerd-fonts.sauce-code-pro
    ];
  };
}
