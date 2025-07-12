{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    programs.vscode = {
      extensions = with pkgs.vscode-extensions; [
        (ms-vscode.cpptools.overrideAttrs ({ meta, ... }: {
          meta = meta // {
            # AArch64 should be supported.
            platforms = meta.platforms ++ [ "aarch64-linux" ];
          };
        }))
        ms-vscode.cmake-tools
      ];

      profiles.default.userSettings = {
        "cmake.configureOnOpen" = true;
      };
    };
  };
}
