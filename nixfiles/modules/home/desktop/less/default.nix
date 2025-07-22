{ lib, config, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    environment = {
      systemPackages.pkgs.sourceHighlight = true;
      variables = {
        LESSOPEN = "/run/current-system/sw/bin/src-hilite-lesspipe.sh %s";
        LESS = " -R ";
      };
    };
  };
}
