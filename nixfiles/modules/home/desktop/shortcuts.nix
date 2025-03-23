{ config, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    dconf.settings = {
      "org/gnome/shell" = {
        favorite-apps = [
          "org.gnome.Nautilus.desktop"
          "com.mitchellh.ghostty.desktop"
          "org.gnome.Ptyxis.desktop"
        ]
        ++ lib.optional config.nova.desktop.browser.enable "chromium-browser.desktop"
        ++ [
          "codium.desktop"
        ];
      };
    };
  };
}
