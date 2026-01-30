{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  imports = [
    ./cpp.nix
    ./editor.nix
    ./nix.nix
    ./python.nix
    ./urdf.nix
  ];

  config = lib.mkIf cfg.enable {
    programs.vscode = {
      enable = true;
      package = pkgs.vscodium;
      profiles.default = {
        enableUpdateCheck = false;
        enableExtensionUpdateCheck = false;
      };
      mutableExtensionsDir = false;
    };
  };
}
