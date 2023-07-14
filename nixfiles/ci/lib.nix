{ supportedSystems
, nixpkgs
, src
, ...
}@args:

rec {
  releaseLib = import (nixpkgs + /pkgs/top-level/release-lib.nix) { inherit supportedSystems; };

  mkNova = pkgs: import src {
    inherit pkgs;
    repos = map (repo: args.${repo}) (builtins.attrNames (import ./nova-repos.nix));
  };
  novaFor = system: mkNova (releaseLib.pkgsFor system);
  novaForAllSystems = f: releaseLib.forAllSystems (system: f (novaFor system));
}
