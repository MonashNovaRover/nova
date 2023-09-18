{ supportedSystems
, nixpkgs
, home-manager
, nixfiles
, enableCompression ? true
, extraModules ? [ ]
, ...
}@args:

let
  ciLib = import ../lib.nix {
    inherit
      supportedSystems
      nixpkgs
      nixfiles;
    repoNames = [ ];
  };

  mkImage = system:
    let
      baseSystem = (import ("${nixpkgs}/nixos/lib/eval-config.nix") {
        inherit system;
        modules = [
          (nixfiles + "/nixos")
          ../../nixos/installer/docker
          ({ pkgs, lib, ... }:
            {
              # The configuration is too complicated to express in static module
              # files.
              installer.cloneConfig = false;

              nova = {
                substituters.nova.password = "***REMOVED***";
                desktop.enable = false;
              };

              home-manager.sharedModules = [
                {
                  # The Home Manager manual causes issues on Hydra.
                  manual = {
                    html.enable = false;
                    manpages.enable = false;
                    json.enable = false;
                  };
                }
              ];
            })
        ] ++ extraModules;
      });

      # "extensions" cannot be used as the variable name due to https://github.com/NixOS/nix/issues/8701.
      extensions' = [
        (prev: [{
          system.build.tarball = ciLib.releaseLib.pkgs.lib.mkForce (prev.system.build.tarball.override
            (ciLib.releaseLib.pkgs.lib.optionalAttrs
              ((ciLib.releaseLib.pkgs.lib.systems.elaborate system).isx86_64 && (ciLib.releaseLib.pkgs.lib.systems.elaborate builtins.currentSystem).isAarch64)
              {
                # Some build tools are prohibitively slow in QEMU.
                # We can use the host tools to build the ISO.
                inherit (ciLib.releaseLib.pkgsFor builtins.currentSystem)
                  pixz;
              }
            // ciLib.releaseLib.pkgs.lib.optionalAttrs (!enableCompression) {
              compressionExtension = "";
              compressCommand = "cat";
              extraInputs = [ ];
            }));
        }])
      ];

      config = (builtins.foldl'
        (system: mkModules: system.extendModules { modules = mkModules system.config; })
        baseSystem
        extensions'
      ).config;
    in
    config.system.build.tarball.overrideAttrs ({ meta ? { }, ... }: {
      meta = meta // rec {
        timeout = 60 * 60 * 10;
        maxSilent = timeout; # The compression is silent, and can take a long time.
      };
    });

  dockerJobs = ciLib.releaseLib.forAllSystems (system: { base = mkImage system; });
in
dockerJobs
