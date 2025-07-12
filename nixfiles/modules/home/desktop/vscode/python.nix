{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    programs.vscode.profiles.default = {
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
        # Use Python from the environment. By default, the Python extension will
        # search the filesystem for Python binaries, which does not work well
        # with the Nix store.
        # This is a simple wrapper that uses Python from PATH.
        "python.defaultInterpreterPath" = "${pkgs.runCommand "python-env-launcher" { nativeBuildInputs = with pkgs; [ makeWrapper ]; } ''
          mkdir -p "$out/bin"
          makeWrapper '${pkgs.coreutils}/bin/env' "$out/bin/python" \
            --add-flags python
        ''}/bin/python";

        "python.experiments.enabled" = false;
      };
    };
  };
}
