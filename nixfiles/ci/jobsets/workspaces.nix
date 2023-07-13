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

  packageLists = lib.novaForAllSystems (nova:
    let
      inherit (import ../workspaces.nix { inherit lib nova; })
        workspace
        hydraPatchedWorkspace;
    in
    # Build Nova Rover packages.
    hydraPatchedWorkspace.novaPackages

    # Build other workspace packages.
    ++ hydraPatchedWorkspace.extraPackages

    # Build development dependencies of Nova packages.
    # This ensures that all the software needed to develop Nova software is
    # available.
    #
    # The non-patched workspace is used here, to build regular development
    # dependencies.
    ++ map (pkg: (pkg.overrideAttrs ({ name, ... }: { name = "${name}-inputs"; })).inputDerivation) workspace.novaPackages

    # Build software in the workspace development environment.
    # While this will mostly overlap in scope with the line above, the workspace
    # development environment contains additional tools that aren't needed to
    # build individual packages, like Colcon.
    #
    # The non-patched workspace is used here, to build regular development
    # dependencies.
    ++ [ (workspace.env.overrideAttrs ({ ... }: { name = "workspace-inputs"; })).inputDerivation ]);

  packageJobs = builtins.mapAttrs
    (system: packageList:
      builtins.listToAttrs
        (map
          (pkg: lib.releaseLib.pkgs.lib.nameValuePair (lib.releaseLib.pkgs.lib.getName pkg) pkg)
          packageList))
    packageLists;
in
packageJobs
