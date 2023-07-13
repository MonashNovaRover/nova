{ supportedSystems
, nixpkgs
, src
, rover
, cameras2
, gui
, coms_utils
}:

let
  novaRepos = [
    rover
    cameras2
    gui
    coms_utils
  ];

  lib = import ../lib.nix {
    inherit
      supportedSystems
      nixpkgs
      src
      novaRepos;
  };

  mkIso = nova:
    let
      inherit (import ../workspaces.nix { inherit lib nova; })
        workspace
        hydraPatchedWorkspace;
    in
    { graphical ? false, includeWorkspace ? false }:
    (import (nixpkgs + /nixos/lib/eval-config.nix) {
      system = nova.pkgs.hostPlatform.system;
      modules = [
        nova.nixosModule
        ../../nixos/installer/cd-dvd/nova-installation-cd-${if graphical then "graphical" else "base"}.nix
        {
          isoImage = {
            isoBaseName = "nixos-nova${lib.releaseLib.pkgs.lib.optionalString graphical "-graphical"}${lib.releaseLib.pkgs.lib.optionalString includeWorkspace "-workspace"}";
            compressImage = true;
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
    }).config.system.build.isoImage;

  isoJobs = lib.novaForAllSystems (nova: {
    iso-base = mkIso nova { };
    iso-base-workspace = mkIso nova { includeWorkspace = true; };
    iso-graphical = mkIso nova { graphical = true; };
    iso-graphical-workspace = mkIso nova { graphical = true; includeWorkspace = true; };
  });
in
isoJobs
