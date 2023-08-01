{ supportedSystems
, nixpkgs
, src
, rosDistro
, ...
}@args:

let
  lib = import ../lib.nix args;

  packageJobs = lib.novaForAllSystems (nova:
    let
      inherit (import ../workspaces.nix { inherit lib nova rosDistro; })
        workspace
        hydraPatchedWorkspace;
    in
    # Build our packages.
    hydraPatchedWorkspace.devPackages

    # Build other workspace packages.
    // hydraPatchedWorkspace.prebuiltPackages

    # Build development dependencies of our packages.
    # This ensures that all the software needed to develop our software is
    # available.
    #
    # The non-patched workspace is used here, to build regular development
    # dependencies.
    // lib.releaseLib.pkgs.lib.mapAttrs' (name: pkg: lib.releaseLib.pkgs.lib.nameValuePair "${name}-inputs" pkg.inputDerivation) workspace.devPackages

    # Build software in the workspace development environment.
    # While this will mostly overlap in scope with the line above, the workspace
    # development environment contains additional tools that aren't needed to
    # build individual packages, like Colcon.
    #
    # The non-patched workspace is used here, to build regular development
    # dependencies.
    // { nova-workspace-inputs = workspace.env.inputDerivation; }

    # Build the workspace itself.
    // { nova-workspace = hydraPatchedWorkspace; });
in
packageJobs
