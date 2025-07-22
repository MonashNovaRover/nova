{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    home.packages = with pkgs; [
      sourceHighlight
    ];
    programs.less = {
      enable = true;
      package = pkgs.less;
    };
    home.sessionVariables = {
      LESSOPEN = "${pkgs.sourceHighlight}/bin/src-hilite-lesspipe.sh %s"; # Syntax highlighting in less
    };
  };
}
