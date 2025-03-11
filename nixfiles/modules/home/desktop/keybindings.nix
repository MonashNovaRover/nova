{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    dconf.settings =
      let
        keybinds = {
          terminal = {
            name = "Terminal";
            command = "ghostty";
            binding = "<Control><Alt>t";
          };
        };
      in
      lib.mkMerge [
        {
          "org/gnome/settings-daemon/plugins/media-keys".custom-keybindings =
            map
              (name: "/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/${name}/")
              (builtins.attrNames keybinds);
        }
        (lib.mapAttrs' (name: lib.nameValuePair "org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/${name}") keybinds)
      ];
  };
}
