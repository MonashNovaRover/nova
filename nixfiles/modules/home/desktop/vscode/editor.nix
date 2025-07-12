{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    programs.vscode = {
      extensions = with pkgs.vscode-extensions; [
        github.copilot
      ];

      profiles.default.userSettings = {
        "workbench.startupEditor" = "none";
        "git.openRepositoryInParentFolders" = "never";
      };
    };
  };
}
