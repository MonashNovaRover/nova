{ supportedSystems
, nixpkgs
, nixpkgs-stable
, home-manager
, src
, enableCompression ? true
, extraModules ? [ ]
, ...
}@args:

let
  lib = import ../lib.nix args;
  mkWorkspaces = args: import ../workspaces.nix ({ inherit lib; } // args);

  mkIso = system:
    { graphical ? false, includeWorkspace ? false }:
    let
      baseSystem = (import ("${nixpkgs}/nixos/lib/eval-config.nix") {
        inherit system;
        modules = [
          (src + /nixos)
          ../../nixos/installer/cd-dvd/nova-installation-cd-${if graphical then "graphical" else "base"}.nix
          ({ pkgs, lib, ... }: {
            nixpkgs.overlays = lib.mkAfter [
              # HACK: We need to apply to QEMU workarounds applied in the
              # patched workspace to the whole package set, as some of the non-
              # ROS packages are used outside the workspace.
              (self: super: {
                nova = super.nova.appendOverlays [
                  (novaSelf: novaSuper: {
                    inherit (mkWorkspaces { novaPkgs = novaSuper; }) hydraPatchedWorkspace;
                    inherit (novaSelf.hydraPatchedWorkspace.novaPackages // novaSelf.hydraPatchedWorkspace.extraPackages)
                      nova-gui-frontend
                      nova-gui-frontend-server;
                  })
                ];
              })

              (self: super:
                let
                  stablePkgs = (import ("${nixpkgs-stable}/pkgs/top-level/release-lib.nix") { inherit supportedSystems; }).pkgsFor self.hostPlatform.system;
                in
                {
                  # https://github.com/NixOS/nixpkgs/issues/245915
                  grub2 = stablePkgs.grub2;
                })
            ];

            isoImage = {
              isoBaseName = "nixos-nova${lib.optionalString graphical "-graphical"}${lib.optionalString includeWorkspace "-workspace"}";
              squashfsCompression = lib.mkIf (!enableCompression) "xz -noI -noD -noF -noX";

              # Add workspace development dependencies, so that the live
              # environment can be used for development right away.
              # The difference between the workspace build and runtime input
              # closures is around ~600MB uncompressed at the time of writing.
              #
              # Note: The Hydra-patched-workspace is not used here, because it
              # will not match the development environment.
              # This means that some packages may fail to build, and will need
              # to be built manually and pushed to Hydra's Nix store.
              storeContents = lib.mkIf includeWorkspace ([
                pkgs.nova.workspace.env.inputDerivation

                # Add the build inputs of the ROS environment of a basic Nova
                # ROS workspace with no Nova packages.
                # nix-ros-workspace has features to generate custom environment
                # derivations for the development of individual packages.
                # We do not need to pre-generate these environment generations,
                # as they can be built offline so long as all the required build
                # inputs are available.
                # Those build inputs consist of the workspace development
                # packages (which can be built offline using the workspace input
                # derivation added above), the workspace prebuilt packages
                # (which are also added above) and the build inputs of the ROS
                # environment derivation, which have not yet been added.
                # We can find the missing inputs by creating an empty ROS
                # environment.
                (pkgs.nova.workspace.override { novaPackages = { }; }).rosEnv.inputDerivation
              ] ++ (builtins.attrValues pkgs.nova.nova.inputs));
            };

            # The configuration is too complicated to express in static module
            # files.
            installer.cloneConfig = false;

            nova = {
              substituters.nova.password = "***REMOVED***";
              desktop.enable = graphical;
              workspace = {
                enable = includeWorkspace;
                package = pkgs.nova.hydraPatchedWorkspace;
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

                # Ship Nova package sources, so that the live environment can be
                # used right away.
                nova.workspace.sources = lib.mkIf includeWorkspace {
                  enable = true;
                  inherit src;
                  external = builtins.mapAttrs (category: builtins.mapAttrs (repo: branch: args.${repo})) (import ../nova-repos.nix);
                };
              })
            ];
          })
        ] ++ extraModules;
      });

      # "extensions" cannot be used as the variable name due to https://github.com/NixOS/nix/issues/8701.
      extensions' = lib.releaseLib.pkgs.lib.optionals ((lib.releaseLib.pkgs.lib.systems.elaborate system).isx86_64 && (lib.releaseLib.pkgs.lib.systems.elaborate builtins.currentSystem).isAarch64) [
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

  isoJobs = lib.releaseLib.forAllSystems (system: {
    iso-base = mkIso system { };
    iso-base-workspace = mkIso system { includeWorkspace = true; };
    iso-graphical = mkIso system { graphical = true; };
    iso-graphical-workspace = mkIso system { graphical = true; includeWorkspace = true; };
  });
in
isoJobs
