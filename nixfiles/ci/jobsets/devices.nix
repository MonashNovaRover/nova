{ supportedSystems
, nixpkgs
, home-manager
, nova-monorepo
, ...
}:

let
  nixfiles = nova-monorepo + "/nixfiles";
  lib = import ../lib.nix {
    inherit
      supportedSystems
      nixpkgs
      nova-monorepo;
    repoNames = [ ];
  };

  mkDeviceSystem = deviceModule: (import "${nixpkgs}/nixos/lib/eval-config.nix" {
    modules = [
      (nixfiles + "/modules/nixos")
      deviceModule
      ({ config, ... }: {
        # Cross-compilation here is not useful, as these jobs are designed to
        # populate a cache to make rebuilds on the devices themselves faster.
        nixpkgs.buildPlatform = config.nixpkgs.hostPlatform;

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
{
  metabox-n850hk = mkDeviceSystem { devices.metabox-n850hk.enable = true; };
  metabox-v158pnh = mkDeviceSystem { devices.metabox-v158pnh.enable = true; };
  jetson-orin-nano-devkit = mkDeviceSystem { devices.jetson = { orin-nano.enable = true; devkit.enable = true; }; };
}
