# Fixes that haven't yet made it into Nixpkgs.

{ config, lib, ... }:

lib.mkMerge [
  # https://github.com/NixOS/nixpkgs/issues/103746#issuecomment-945091229
  (lib.mkIf config.services.xserver.displayManager.gdm.enable {
    systemd.services."getty@tty1".enable = false;
    systemd.services."autovt@tty1".enable = false;
  })
]
