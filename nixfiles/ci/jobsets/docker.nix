{ supportedSystems
, nixpkgs
, home-manager
, nova-monorepo
, extraModules ? [ ]
, ...
}@args:

let
  nixfiles = nova-monorepo + "/nixfiles";
  ciLib = import ../lib.nix {
    inherit
      supportedSystems
      nixpkgs
      nova-monorepo;
    repoNames = [ ];
  };

  mkImageJob = system:
    let
      nixos = import (nixpkgs + "/nixos") {
        configuration = { pkgs, lib, ... }: {
          imports = [
            (nixfiles + "/nixos")
            (nixfiles + "/nixos/installer/docker")
          ];

          nixpkgs.hostPlatform = system;

          # The configuration is too complicated to express in static module
          # files.
          installer.cloneConfig = false;

          nova = {
            substituters.nova.password = builtins.readFile ../../external/src/other/secrets/hydra-password.txt;
            
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
        };
      };

      image = ciLib.pkgs.dockerTools.buildImage {
        name = "nova-nixos";
        tag = "latest";
        # https://github.com/NixOS/nixpkgs/blob/f63a5ba18f3b1b9cce1a9a68d33330989978edae/pkgs/build-support/docker/default.nix#L77C35
        architecture = nixos.pkgs.go.GOARCH;

        extraCommands = ''
          # Copy the Nix store registration list.
          # The Docker container module loads store registrations upon boot.
          # This is better than the dockerTools buildImageWithNixDb implementation as buildImageWithNixDb creates GC roots, which is not appropriate here.
          ln -s '${ciLib.pkgs.closureInfo { rootPaths = [ nixos.config.system.build.toplevel ]; }}/registration' nix-path-registration

          # Make the init script available in the root directory.
          # This allows the latest init script to be used as an entrypoint.
          # The Docker container module updates this link upon activation.
          ln -s '${nixos.config.system.build.toplevel}/init' init
        '';

        config.Cmd = [ "/init" ];
      };
    in
    ciLib.pkgs.runCommand "docker-image-hydra" { } ''
      mkdir -p "$out/docker-image"
      ln -s '${image}' "$out/docker-image"

      mkdir -p "$out/nix-support"
      echo "file docker-image $out/docker-image/${baseNameOf image}" > "$out/nix-support/hydra-build-products"
    '';

  dockerJobs = ciLib.releaseLib.forAllSystems (system: { base = mkImageJob system; });
in
dockerJobs
