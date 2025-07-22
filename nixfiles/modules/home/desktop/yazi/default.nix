{ config, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    programs.yazi = {
      enable = true;
      settings = {
        yazi = {
          mgr = {
            sort_by = "alphabetical";
            sort_sensitive = true;
            sort_dir_first = true;
            linemode = "size";
            show_symlink = true;
          };
          preview = {
            wrap = "yes";
            tab_size = 8;
            image_delay = 16;
            image_filter = "catmull-rom";
            image_quality = 80;
            sixel_fraction = 20;
          };
          opener.open = [
            { run = "xdg-open \"$@\""; desc = "Open"; }
          ];
        };
        theme = lib.importTOML ./yaziTheme.toml;
      };
    };
  };
}
