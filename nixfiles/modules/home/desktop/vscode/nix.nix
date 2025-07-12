{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    programs.vscode = {
      extensions = with pkgs.vscode-extensions; [
        jnoortheen.nix-ide
      ];

      profiles.default.userSettings = {
        "nix.enableLanguageServer" = true;
        "nix.serverPath" = "${pkgs.nixd}/bin/nixd";
        "nix.serverSettings".nixd = {
          formatting.command = "${pkgs.nixpkgs-fmt}/bin/nixpkgs-fmt";
        };
        "[nix]" = {
          "editor.formatOnSave" = true;
          "editor.formatOnPaste" = true;
        };
      };
    };
  };
}
