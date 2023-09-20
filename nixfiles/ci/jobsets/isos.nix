{ supportedSystems
, nixpkgs
, nixpkgs-stable
, home-manager
, jetpack-nixos
, nixfiles
, enableCompression ? true
, extraModules ? [ ]
, ...
}@args:

let
  ciLib = import ../lib.nix args;

  mkIso = system:
    { graphical ? false, includeWorkspace ? false, extraPlatformModules ? [ ] }:
    let
      baseSystem = (import ("${nixpkgs}/nixos/lib/eval-config.nix") {
        inherit system;
        modules = [
          (nixfiles + "/nixos")
          ../../nixos/installer/cd-dvd/nova-installation-cd-${if graphical then "graphical" else "base"}.nix
          ({ pkgs, lib, ... }:
            let
              inherit (import ../workspaces.nix { lib = ciLib; novaPkgs = pkgs.nova; })
                workspace
                hydraPatchedWorkspace;
            in
            {
              nixpkgs.overlays = lib.mkAfter [
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
                isoBaseName = "nixos${lib.optionalString graphical "-graphical"}${lib.optionalString includeWorkspace "-workspace"}";
                squashfsCompression = lib.mkIf (!enableCompression) "xz -noI -noD -noF -noX";

                # Add workspace development dependencies, so that the live
                # environment can be used for development right away.
                # The difference between the workspace build and runtime input
                # closures is around ~600MB uncompressed at the time of writing.
                #
                # Hydra-patched extraPackages are used, as we do not care about
                # their development dependencies.
                # The original novaPackages must be used to ensure that the
                # development dependencies are relevant.
                storeContents = lib.mkIf includeWorkspace ([
                  (workspace.override { inherit (hydraPatchedWorkspace) extraPackages; }).env.inputDerivation

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
                  (workspace.override { novaPackages = { }; extraPackages = { }; }).rosEnv.inputDerivation
                ] ++ (builtins.attrValues pkgs.nova.nova.inputs));
              };

              # The configuration is too complicated to express in static module
              # files.
              installer.cloneConfig = false;

              # The installation profiles configure their own networking.
              networking.networkmanager.enable = lib.mkForce false;

              nova = {
                inherit (ciLib) repos;
                substituters.nova.password = "***REMOVED***";
                desktop.enable = graphical;
                workspace = {
                  enable = includeWorkspace;
                  package = hydraPatchedWorkspace;
                  services.gui.frontendPackage = hydraPatchedWorkspace.novaPackages.nova-gui-frontend;
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
                    src = nixfiles;
                    external = builtins.mapAttrs (category: builtins.mapAttrs (repo: branch: args.${repo})) (import ../nova-repos.nix);
                  };
                })
              ];
            })
        ] ++ extraModules ++ extraPlatformModules;
      });

      # "extensions" cannot be used as the variable name due to https://github.com/NixOS/nix/issues/8701.
      extensions' = ciLib.releaseLib.pkgs.lib.optionals ((ciLib.releaseLib.pkgs.lib.systems.elaborate system).isx86_64 && (ciLib.releaseLib.pkgs.lib.systems.elaborate builtins.currentSystem).isAarch64) [
        # Some build tools are prohibitively slow in QEMU.
        # We can use the host tools to build the ISO.
        (prev: [{
          system.build.squashfsStore = ciLib.releaseLib.pkgs.lib.mkForce (prev.system.build.squashfsStore.override {
            inherit (ciLib.releaseLib.pkgsFor builtins.currentSystem)
              squashfsTools;
          });
        }])
        (prev: [{
          system.build.isoImage = ciLib.releaseLib.pkgs.lib.mkForce (prev.system.build.isoImage.override {
            inherit (ciLib.releaseLib.pkgsFor builtins.currentSystem)
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
    config.system.build.isoImage.overrideAttrs ({ meta ? { }, ... }: {
      meta = meta // rec {
        timeout = 60 * 60 * 10;
        maxSilent = timeout; # The SquashFS compression is silent, and can take a long time.
      };
    });

  mkJetsonIso = deviceConfig: { extraPlatformModules ? [ ], ... }@args: mkIso "aarch64-linux" (args // {
    extraPlatformModules = extraPlatformModules ++ [
      ({ lib, ... }: {
        devices.jetson = deviceConfig;

        # https://github.com/anduril/jetpack-nixos/blob/57b79aba8d4608839f9777a775bfcb1354d88f21/flake.nix#L15C3
        disabledModules = [ "profiles/all-hardware.nix" ];

        # Disable incompatible VM guest drivers.
        virtualisation.hypervGuest.enable = lib.mkForce false;
      })
    ];
  });

  mkIsoJobs = mkPlatformIso: { includeGraphical ? true }: {
    iso-base = mkPlatformIso { };
    iso-base-workspace = mkPlatformIso { includeWorkspace = true; };
  } // ciLib.releaseLib.pkgs.lib.optionalAttrs includeGraphical {
    iso-graphical = mkPlatformIso { graphical = true; };
    iso-graphical-workspace = mkPlatformIso { graphical = true; includeWorkspace = true; };
  };

  isoJobs =
    (ciLib.releaseLib.forAllSystems (system: mkIsoJobs (mkIso system) { }))
    // {
      jetson-orin-nano-devkit = mkIsoJobs
        (mkJetsonIso {
          orin-nano.enable = true;
          devkit.enable = true;
        })
        { includeGraphical = false; };
    };
in
isoJobs
