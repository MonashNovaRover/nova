{ supportedSystems
, nixpkgs
, src
, repos ? builtins.foldl' (repos: category: repos ++ builtins.attrNames category) [ ] (builtins.attrValues (import ./nova-repos.nix))
, ...
}@args:

rec {
  releaseLib = import ("${nixpkgs}/pkgs/top-level/release-lib.nix") { inherit supportedSystems; };

  mkNova = pkgs: import src {
    inherit pkgs;
    repos = map (repo: args.${repo}) repos;
  };
  novaFor = system: mkNova (releaseLib.pkgsFor system);
  novaForAllSystems = f: releaseLib.forAllSystems (system: f (novaFor system));
}
