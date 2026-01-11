{ config, pkgs, lib, ... }:

let
  cfg = config.nova.workspace.sources;
in
{
  options.nova.workspace.sources = {
    enable = lib.mkOption {
      type = with lib.types; bool;
      description = ''
        Pre-install Nova Rover package source code, and copy it to the home
        directory upon activation.

        This option is used by premade VM and ISO images.
      '';
      default = false;
    };
    src = lib.mkOption {
      type = with lib.types; path;
      description = ''
        The root of the source tree.
      '';
    };
    external = lib.mkOption {
      type = with lib.types; attrsOf (attrsOf path);
      description = ''
        A mapping of package categories to their source directories.
      '';
      default = { };
    };
  };

  config = lib.mkIf cfg.enable {
    home.activation.workspace-source-setup = lib.hm.dag.entryAfter [ "writeBoundary" ] (
      ''
        if [ ! -e ~/nova/nixfiles ] && [ ! -e ~/nova/src ]; then
          echo 'Populating initial workspace source tree...'
          cp -r '${cfg.src}' ~/nova/nixfiles
          chmod -R u+w ~/nova/nixfiles

          mkdir -p ~/nova/src
          ${builtins.concatStringsSep "\n" (lib.mapAttrsToList
            (category: repos: builtins.concatStringsSep "\n"
              ([ "mkdir -p ~/nova/src/'${category}'" ] ++
              (lib.mapAttrsToList
                (repoName: repo: "cp -r '${repo}' ~/nova/src/'${category}'/'${repoName}'")
                repos)))
            cfg.external)}
          chmod -R u+w ~/nova/src
        fi
      ''
    );
  };
}
