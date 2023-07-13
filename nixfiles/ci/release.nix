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

  lib = import (nixpkgs + /pkgs/top-level/release-lib.nix) { inherit supportedSystems; };
  pkgs = lib.pkgs;

  mkNova = pkgs: import src { inherit pkgs; repos = novaRepos; };
  novaFor = system: mkNova (lib.pkgsFor system);
  novaForAllSystems = f: lib.forAllSystems (system: f (novaFor system));


  packageLists = novaForAllSystems (nova:
    let
      workspace = nova.pkgs.ros.nova-workspace;
      hydraPatchedWorkspace = workspace.override
        # Some x86_64 packages fail to build in QEMU on Aarch64. Workarounds
        # must be made to avoid these failures.
        # There is no easy way at this stage to determine if Hydra has access to
        # any real x86_64 machines, so these changes will apply indiscriminately.
        (pkgs.lib.optionalAttrs ((pkgs.lib.systems.elaborate builtins.currentSystem).isAarch64 && nova.pkgs.hostPlatform.isx86_64) {
          novaPackages =
            let
              # The GUI frontend fails to build, but the output contains only
              # static Web assets, and is architecture-independent. Use the
              # Aarch64 version instead.
              nova-gui-frontend = (novaFor "aarch64-linux").pkgs.nova-gui-frontend;
              nova-gui-frontend-server = nova.pkgs.nova-gui-frontend-server.override { inherit nova-gui-frontend; };

              added = [
                nova-gui-frontend
                nova-gui-frontend-server
              ];
              removed = [
                nova.pkgs.nova-gui-frontend
                nova.pkgs.nova-gui-frontend-server
              ];

              removedNames = map pkgs.lib.getName removed;
            in
            (builtins.filter (pkg: !builtins.elem (pkgs.lib.getName pkg) removedNames) workspace.novaPackages) ++ added;
        });
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
    ++ map (pkg: pkg.inputDerivation) workspace.novaPackages

    # Build software in the workspace development environment.
    # While this will mostly overlap in scope with the line above, the workspace
    # development environment contains additional tools that aren't needed to
    # build individual packages, like Colcon.
    #
    # The non-patched workspace is used here, to build regular development
    # dependencies.
    ++ [ workspace.env.inputDerivation ]);

  packageJobs = builtins.mapAttrs
    (system: packageList:
      builtins.listToAttrs
        (map
          (pkg: pkgs.lib.nameValuePair (pkgs.lib.getName pkg) pkg)
          packageList))
    packageLists;
in
packageJobs
