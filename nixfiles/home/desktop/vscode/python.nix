{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    programs.vscode = {
      extensions = with pkgs.vscode-extensions; [
        (ms-python.python.overrideAttrs ({ meta, ... }: {
          meta = meta // {
            # There are no debugging tools for AArch64, but hopefully everything
            # else works.
            platforms = meta.platforms ++ [ "aarch64-linux" ];
          };
        }))
        ms-python.vscode-pylance
      ];

      userSettings = {
        "python.experiments.enabled" = false;
      };
    };
  };
}
