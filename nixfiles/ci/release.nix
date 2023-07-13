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
    # Build Nova Rover packages.
    nova.pkgs.ros.nova-workspace.novaPackages

    # Build other workspace packages.
    ++ nova.pkgs.ros.nova-workspace.extraPackages

    # Build development dependencies of Nova packages.
    # This ensures that all the software needed to develop Nova software is
    # available.
    ++ map (pkg: pkg.inputDerivation) nova.pkgs.ros.nova-workspace.novaPackages

    # Build software in the workspace development environment.
    # While this will mostly overlap in scope with the line above, the workspace
    # development environment contains additional tools that aren't needed to
    # build individual packages, like Colcon.
    ++ [ nova.pkgs.ros.nova-workspace.env.inputDerivation ]);
in
packageLists
