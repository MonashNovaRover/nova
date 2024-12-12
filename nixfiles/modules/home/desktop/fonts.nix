{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    fonts.fontconfig.enable = true;
    fonts.fontconfig.defaultFonts.monospace = [ "SauceCodePro" "SourceCodePro" ];

    home.packages = with pkgs; [
      # (nerdfonts.override { fonts = [ "SourceCodePro" ]; })
      nerd-fonts.sauce-code-pro
    ];
  };
}
