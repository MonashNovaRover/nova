{ supportedSystems
, nixpkgs
, nova-monorepo
, repoNames ? builtins.foldl' (repos: category: repos ++ builtins.attrNames category) [ ] (builtins.attrValues (import ./nova-repos.nix))
, ...
}@args:

let
  nixfiles = nova-monorepo + "/nixfiles";
in rec {
  pkgs = import nixpkgs { };
  releaseLib = import (pkgs.path + "/pkgs/top-level/release-lib.nix") { inherit supportedSystems; };

  # repos = map (repo: args.${repo}) repoNames;
  repos = import ../external/default-paths.nix;

  mkNova = pkgs: import nixfiles { inherit pkgs repos; };
  novaFor = system: mkNova (releaseLib.pkgsFor system);
  novaForAllSystems = f: releaseLib.forAllSystems (system: f (novaFor system));
}
