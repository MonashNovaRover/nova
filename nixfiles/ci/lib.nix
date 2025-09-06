{ supportedSystems
, nixpkgs
, nova-monorepo
, repoNames ? null #? builtins.foldl' (repos: category: repos ++ builtins.attrNames category) [ ] (builtins.attrValues (import ./nova-repos.nix))
, ...
}@args:

let
  nixfiles = nova-monorepo + "/nixfiles";
in rec {
  pkgs = import nixpkgs { };
  #releaseLib = import (pkgs.path + "/pkgs/top-level/release-lib.nix") { inherit supportedSystems; };
  releaseLib = import (pkgs.path + "/pkgs/top-level/release-lib.nix") {
    inherit supportedSystems;
    nixpkgsArgs = {
      config = {
        allowAliases = true; # https://github.com/NixOS/nixpkgs/blob/a50c9f77d238909b5f96d97dab0f69d1c9abefa8/pkgs/top-level/release-lib.nix#L9 override this to true
        allowUnfree = false;
        inHydra = true;
      };
      __allowFileset = false;
    };
  };

  # repos = map (repo: args.${repo}) repoNames;
  repos = if repoNames == null
    then import ../external/default-paths.nix
    else map (repo: args.${repo}) repoNames;

  mkNova = pkgs: import nixfiles { inherit pkgs repos; };
  novaFor = system: mkNova (releaseLib.pkgsFor system);
  novaForAllSystems = f: releaseLib.forAllSystems (system: f (novaFor system));
}
