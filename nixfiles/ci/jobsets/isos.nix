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

                home.activation.livecd-workspace-source-setup = lib.mkIf includeWorkspace (lib.hm.dag.entryAfter [ "writeBoundary" ] (
                  let
                    cpRepo = repo: directory:
                      # Symlinks are used here in a very specific way.
                      #
                      # Our repositories contain their own package definitions,
                      # which means that their source directories are copied into
                      # the Nix store from path references. This means that the
                      # name of source derivations is derived from the name of
                      # the source directory itself.
                      #
                      # The source derivations given by Hydra, however, have no
                      # such guarantee. At the time of writing, Git inputs have
                      # a derivation name ending in "source".
                      #
                      # This difference changes the input hashes of many of our
                      # packages. To mitigate this, we create symlinks to
                      # directories with the name of the Hydra source derivation,
                      # so that the directory derivation generated later matches.
                      let
                        prefix = "${directory}/.${baseNameOf args.${repo}}";
                        destination = "${prefix}/${builtins.substring 33 (builtins.stringLength (baseNameOf args.${repo})) (baseNameOf args.${repo})}";
                      in
                      ''
                        mkdir "${prefix}"
                        cp -r ${args.${repo}} "${destination}"
                        chmod -R u+w "${destination}"
                        ln -s "${destination}" "${directory}/${repo}"
                      '';
                  in
                  ''
                    if [ ! -f ~/src ]; then
                      echo 'Populating initial workspace source tree...'
                      ${cpRepo "src" "$HOME"}
                      ${builtins.concatStringsSep "\n" (lib.mapAttrsToList
                        (category: repos: builtins.concatStringsSep "\n"
                          ([ "mkdir -p ~/src/external/src/'${category}'" ] ++
                          (map
                            (repo: cpRepo repo "$HOME/src/external/src/${category}")
                            (builtins.attrNames repos))))
                        (import ../nova-repos.nix))
                      }
                    fi
                  ''
                ));
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
