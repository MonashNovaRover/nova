{ supportedSystems
, nixpkgs
, src
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
          nova.nixosModule
          ../../nixos/installer/cd-dvd/nova-installation-cd-${if graphical then "graphical" else "base"}.nix
          {
            isoImage = {
              isoBaseName = "nixos-nova${lib.releaseLib.pkgs.lib.optionalString graphical "-graphical"}${lib.releaseLib.pkgs.lib.optionalString includeWorkspace "-workspace"}";
            };

            nova = {
              desktop.enable = graphical;
              substituters.nova.password = "***REMOVED***";
            };
            home-manager.sharedModules = [{
              # The Home Manager manual causes issues on Hydra.
              manual = {
                html.enable = false;
                manpages.enable = false;
                json.enable = false;
              };
              nova = {
                workspace = {
                  enable = includeWorkspace;
                  package = hydraPatchedWorkspace;
                };
              };
            }];
          }
        ];
      });

      extensions = lib.releaseLib.pkgs.lib.optionals (nova.pkgs.hostPlatform.isx86_64 && (lib.releaseLib.pkgs.lib.systems.elaborate builtins.currentSystem).isAarch64) [
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
        extensions
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
