{ supportedSystems
, nixpkgs
, home-manager
, nixfiles
}:

let
  lib = import ../lib.nix {
    inherit
      supportedSystems
      nixpkgs
      nixfiles;
    repoNames = [ ];
  };

  mkDeviceSystem = device: (import "${nixpkgs}/nixos/lib/eval-config.nix" {
    modules = [
      (nixfiles + /nixos)
      ({ config, ... }: {
        # Cross-compilation here is not useful, as these jobs are designed to
        # populate a cache to make rebuilds on the devices themselves faster.
        nixpkgs.buildPlatform = config.nixpkgs.hostPlatform;

        devices.${device}.enable = true;

        nova = {
          profile = "shared";
          substituters.nova.enable = false;
        };

        home-manager.sharedModules = [{
          home.stateVersion = config.system.nixos.release;

          # The Home Manager manual causes issues on Hydra.
          manual = {
            html.enable = false;
            manpages.enable = false;
            json.enable = false;
          };
        }];

        fileSystems."/".fsType = "tmpfs";
      })
    ];
  }).config.system.build.toplevel;
in
lib.releaseLib.pkgs.lib.genAttrs [
  "metabox-n850hk"
  "metabox-v158pnh"
]
  mkDeviceSystem
