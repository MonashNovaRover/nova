{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    programs.vscode.profiles.default = {
      extensions = with pkgs.vscode-extensions; [
        (ms-vscode.cpptools.overrideAttrs ({ meta, ... }: {
          meta = meta // {
            # AArch64 should be supported.
            platforms = meta.platforms ++ [ "aarch64-linux" ];
          };
        }))
        ms-vscode.cmake-tools
      ];

      userSettings = {
        "cmake.configureOnOpen" = true;
      };
    };
  };
}
