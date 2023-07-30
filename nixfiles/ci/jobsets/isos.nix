{ supportedSystems
, nixpkgs
, home-manager
, src
, enableCompression ? true
, extraModules ? [ ]
, ...
}@args:

let
  lib = import ../lib.nix args;

  mkIso = nova:
    { graphical ? false, includeWorkspace ? false }:
    let
      inherit (import ../workspaces.nix { inherit lib nova; })
        workspace
        hydraPatchedWorkspace;

      baseSystem = (import (nixpkgs + /nixos/lib/eval-config.nix) {
        system = nova.pkgs.hostPlatform.system;
        modules = [
          (nova.nixosModule.override {
            homeManagerNixOSModule = "${home-manager}/nixos";
          })
          ../../nixos/installer/cd-dvd/nova-installation-cd-${if graphical then "graphical" else "base"}.nix
          ({ lib, ... }: {
            isoImage = {
              isoBaseName = "nixos-nova${lib.optionalString graphical "-graphical"}${lib.optionalString includeWorkspace "-workspace"}";
              squashfsCompression = lib.mkIf (!enableCompression) "xz -noI -noD -noF -noX";

              # Add workspace development dependencies, so that the live
              # environment can be used for development right away.
              # The difference between the workspace build and runtime input
              # closures is around ~600MB uncompressed at the time of writing.
              storeContents = lib.mkIf includeWorkspace ([ workspace.env.inputDerivation ] ++ (builtins.attrValues nova.inputs));
            };

            # The configuration is too complicated to express in static module
            # files.
            installer.cloneConfig = false;

            nova = {
              substituters.nova.password = "***REMOVED***";
              desktop.enable = graphical;
              workspace = {
                enable = includeWorkspace;
                package = hydraPatchedWorkspace;
              };
            };
            home-manager.sharedModules = [
              ({ lib, ... }: {
                # The Home Manager manual causes issues on Hydra.
                manual = {
                  html.enable = false;
                  manpages.enable = false;
                  json.enable = false;
                };

                home.activation.livecd-workspace-source-setup = lib.mkIf includeWorkspace (lib.hm.dag.entryAfter [ "writeBoundary" ] ''
                  if [ ! -f ~/src ]; then
                    echo 'Populating initial workspace source tree...'
                    cp -r '${src}' ~/src
                    chmod -R u+w ~/src
                    ${builtins.concatStringsSep "\n" (lib.mapAttrsToList
                      (category: repos: builtins.concatStringsSep "\n"
                        ([ "mkdir -p ~/src/external/src/'${category}'" ] ++
                        (map
                          (repo: "cp -r '${args.${repo}}' ~/src/external/src/${category}/'${repo}'")
                          (builtins.attrNames repos))))
                      (import ../nova-repos.nix))
                    }
                    chmod -R u+w ~/src
                  fi
                '');
              })
            ];
          })
        ] ++ extraModules;
      });

      # "extensions" cannot be used as the variable name due to https://github.com/NixOS/nix/issues/8701.
      extensions' = lib.releaseLib.pkgs.lib.optionals (nova.pkgs.hostPlatform.isx86_64 && (lib.releaseLib.pkgs.lib.systems.elaborate builtins.currentSystem).isAarch64) [
        # Some build tools are prohibitively slow in QEMU.
        # We can use the host tools to build the ISO.
        (prev: [{
          system.build.squashfsStore = lib.releaseLib.pkgs.lib.mkForce (prev.system.build.squashfsStore.override {
            inherit (lib.releaseLib.pkgsFor builtins.currentSystem)
              squashfsTools;
          });
        }])
        (prev: [{
          system.build.isoImage = lib.releaseLib.pkgs.lib.mkForce (prev.system.build.isoImage.override {
            inherit (lib.releaseLib.pkgsFor builtins.currentSystem)
              xorriso
              zstd;
          });
        }])
      ];

      config = (builtins.foldl'
        (system: mkModules: system.extendModules { modules = mkModules system.config; })
        baseSystem
        extensions'
      ).config;
    in
    config.system.build.isoImage;

  isoJobs = lib.novaForAllSystems (nova: {
    iso-base = mkIso nova { };
    iso-base-workspace = mkIso nova { includeWorkspace = true; };
    iso-graphical = mkIso nova { graphical = true; };
    iso-graphical-workspace = mkIso nova { graphical = true; includeWorkspace = true; };
  });
in
isoJobs
