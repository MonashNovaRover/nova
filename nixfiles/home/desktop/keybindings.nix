{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    dconf.settings =
      let
        keybinds = [
          {
            name = "Terminal";
            command = "blackbox";
            binding = "<Control><Alt>t";
          }
        ];
      in
      lib.mkMerge [
        {
          "org/gnome/settings-daemon/plugins/media-keys".custom-keybindings =
            builtins.genList
              (i: "/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/custom${toString i}/")
              (builtins.length keybinds);
        }
        (builtins.listToAttrs (lib.imap0 (i: lib.nameValuePair "org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/custom${toString i}") keybinds))
      ];
  };
}
