{ supportedSystems
, nixpkgs
, src
, rover
, cameras2
, gui
, coms_utils
}:

let
  lib = import (nixpkgs + /pkgs/top-level/release-lib.nix) { inherit supportedSystems; };
  pkgs = lib.pkgs;

  mkNova = pkgs: import src {
    inherit pkgs;
    repos = [
      rover
      cameras2
      gui
      coms_utils
    ];
  };

  systems = pkgs.lib.intersectLists supportedSystems [
    "x86_64-linux"
    "aarch64-linux"
  ];

  genNovaPlatformRecipes = f: pkgs.lib.genAttrs
    systems
    (system: f (mkNova (lib.pkgsFor system)));
in
builtins.mapAttrs (name: genNovaPlatformRecipes) rec {
  workspace = nova: nova.pkgs.ros.nova-workspace;
  workspace-env-inputs = nova: (workspace nova).env.inputDerivation;
}
