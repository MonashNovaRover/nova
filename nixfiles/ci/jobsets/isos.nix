{ supportedSystems
, nixpkgs
, nixpkgs-stable
, home-manager
, jetpack-nixos
, nixos-hardware
, nova-monorepo
, enableCompression ? true
, extraModules ? [ ]
, ...
}@args:

let
  nixfiles = nova-monorepo + "/nixfiles";
  ciLib = import ../lib.nix args;

  mkIso = system: extraPlatformModules:
    { graphical ? false, includeWorkspace ? false }:
    let
      baseSystem = (import ("${nixpkgs}/nixos/lib/eval-config.nix") {
        inherit system;
        modules = [
          (nixfiles + "/nixos")
          ../../nixos/installer/cd-dvd/nova-installation-cd-${if graphical then "graphical" else "base"}.nix
          ({ config, pkgs, lib, ... }:
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
                isoBaseName = lib.mkForce "nixos${lib.optionalString graphical "-graphical"}${lib.optionalString includeWorkspace "-workspace"}";
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

              # Some device configurations explicitly enable NetworkManager,
              # but the installation profiles configure their own networking.
              # In particular, some use networking.wireless.enable, which is
              # incompatible with NetworkManager. In these cases, it should be
              # disabled.
              networking.networkmanager.enable = lib.mkIf config.networking.wireless.enable (lib.mkForce false);

              nova = {
                inherit (ciLib) repos;
                substituters.nova.password = builtins.readFile ../../external/src/other/secrets/hydra-password.txt;
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
      extensions' = ciLib.pkgs.lib.optionals ((ciLib.pkgs.lib.systems.elaborate system).isx86_64 && (ciLib.pkgs.lib.systems.elaborate builtins.currentSystem).isAarch64) [
        # Some build tools are prohibitively slow in QEMU.
        # We can use the host tools to build the ISO.
        (prev: [{
          system.build.squashfsStore = ciLib.pkgs.lib.mkForce (prev.system.build.squashfsStore.override {
            inherit (ciLib.releaseLib.pkgsFor builtins.currentSystem)
              squashfsTools;
          });
        }])
        (prev: [{
          system.build.isoImage = ciLib.pkgs.lib.mkForce (prev.system.build.isoImage.override {
            inherit (ciLib.releaseLib.pkgsFor builtins.currentSystem)
              xorriso
              zstd;
          });
        }])
      ];

      eval = (builtins.foldl'
        (system: mkModules: system.extendModules { modules = mkModules system.config; })
        baseSystem
        extensions'
      );
    in
    eval.config.system.build.isoImage.overrideAttrs ({ passthru ? { }, meta ? { }, ... }: {
      passthru = passthru // {
        inherit (eval) config options;
      };
      meta = meta // rec {
        timeout = 60 * 60 * 10;
        maxSilent = timeout; # The SquashFS compression is silent, and can take a long time.
      };
    });

  mkJetsonIso = deviceConfig: mkIso "aarch64-linux" [
    ({ lib, ... }: {
      devices.jetson = deviceConfig;

      # https://github.com/anduril/jetpack-nixos/blob/57b79aba8d4608839f9777a775bfcb1354d88f21/flake.nix#L15C3
      disabledModules = [ "profiles/all-hardware.nix" ];

      # Disable incompatible VM guest drivers.
      virtualisation.hypervGuest.enable = lib.mkForce false;
    })
  ];

  mkMacbookT2Iso = mkIso "x86_64-linux" [
    ({ modulesPath, ... }: {
      imports = [
        (nixos-hardware + "/apple")
        (nixos-hardware + "/apple/t2")

        # Disable ZFS - the kernel version is not always supported.
        (modulesPath + "/installer/cd-dvd/installation-cd-minimal-new-kernel-no-zfs.nix")
      ];

      disabledModules = [
        # Imported by installation-cd-minimal-new-kernel-no-zfs.nix.
        # We want to disable ZFS, but we don't want the new kernel.
        "installer/cd-dvd/installation-cd-minimal-new-kernel.nix"
      ];
    })
  ];

  mkIsoJobs = mkPlatformIso: { includeWorkspace ? true, includeGraphical ? true }: {
    iso-base = mkPlatformIso { };
  } // ciLib.pkgs.lib.optionalAttrs includeWorkspace {
    iso-base-workspace = mkPlatformIso { includeWorkspace = true; };
  } // ciLib.pkgs.lib.optionalAttrs includeGraphical ({
    iso-graphical = mkPlatformIso { graphical = true; };
  } // ciLib.pkgs.lib.optionalAttrs includeWorkspace {
    iso-graphical-workspace = mkPlatformIso { graphical = true; includeWorkspace = true; };
  });

  genericIsoJobs = ciLib.releaseLib.forAllSystems (system: mkIsoJobs (mkIso system [ ]) { });

  customIsoJobs = {
    jetson-orin-nano-devkit = mkIsoJobs
      (mkJetsonIso {
        orin-nano.enable = true;
        devkit.enable = true;
      })
      {
        includeGraphical = false;
      };

    macbook-t2 = mkIsoJobs mkMacbookT2Iso {
      # Disable workspace variants to save resources. There's not much of a
      # usecase for Macbooks.
      includeWorkspace = false;

      # Macbook users will need to do some tricky bootloader setup anyway;
      # they can handle some manual partitioning.
      includeGraphical = false;
    };
  };

  isoJobs = genericIsoJobs // customIsoJobs;
in
isoJobs
