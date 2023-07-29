{ supportedSystems
, nixpkgs
, src
, rosDistro
, ...
}@args:

let
  lib = import ../lib.nix args;

  packageLists = lib.novaForAllSystems (nova:
    let
      inherit (import ../workspaces.nix { inherit lib nova rosDistro; })
        workspace
        hydraPatchedWorkspace;
    in
    # Build our packages.
    hydraPatchedWorkspace.devPackages

    # Build other workspace packages.
    ++ hydraPatchedWorkspace.prebuiltPackages

    # Build development dependencies of our packages.
    # This ensures that all the software needed to develop our software is
    # available.
    #
    # The non-patched workspace is used here, to build regular development
    # dependencies.
    ++ map (pkg: (pkg.overrideAttrs ({ name, ... }: { name = "${name}-inputs"; })).inputDerivation) workspace.devPackages

    # Build software in the workspace development environment.
    # While this will mostly overlap in scope with the line above, the workspace
    # development environment contains additional tools that aren't needed to
    # build individual packages, like Colcon.
    #
    # The non-patched workspace is used here, to build regular development
    # dependencies.
    ++ [ (workspace.env.overrideAttrs ({ ... }: { name = "workspace-inputs"; })).inputDerivation ]

    # Build the workspace itself.
    ++ [ hydraPatchedWorkspace ]);

  packageJobs = builtins.mapAttrs
    (system: packageList:
      builtins.listToAttrs
        (map
          (pkg: lib.releaseLib.pkgs.lib.nameValuePair (lib.releaseLib.pkgs.lib.getName pkg) pkg)
          packageList))
    packageLists;
in
packageJobs
