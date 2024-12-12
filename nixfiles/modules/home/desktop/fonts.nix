{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    fonts.fontconfig.enable = true;
    fonts.fontconfig.defaultFonts.monospace = [ "SauceCodePro" "SourceCodePro" ];

    home.packages = with pkgs; [
<<<<<<< HEAD
      nerd-fonts._0xproto
=======
      # (nerdfonts.override { fonts = [ "SourceCodePro" ]; })
>>>>>>> 4116677 (fix: added sauce-code-pro using home manager)
      nerd-fonts.sauce-code-pro
    ];
  };
}
