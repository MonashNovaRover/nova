{ supportedSystems
, nixpkgs
, nixfiles
, repoNames ? builtins.foldl' (repos: category: repos ++ builtins.attrNames category) [ ] (builtins.attrValues (import ./nova-repos.nix))
, ...
}@args:

rec {
  releaseLib = import ("${nixpkgs}/pkgs/top-level/release-lib.nix") { inherit supportedSystems; };

  repos = map (repo: args.${repo}) repoNames;

  mkNova = pkgs: import nixfiles { inherit pkgs repos; };
  novaFor = system: mkNova (releaseLib.pkgsFor system);
  novaForAllSystems = f: releaseLib.forAllSystems (system: f (novaFor system));
}
