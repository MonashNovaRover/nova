{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    programs.vscode.profiles.default = {
      extensions = with pkgs.vscode-extensions; [
        ms-dotnettools.csharp
      ];
    };
  };
}
