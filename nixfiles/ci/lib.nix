{ supportedSystems
, nixpkgs
, src
, novaRepos ? [ ]
}:

rec {
  releaseLib = import (nixpkgs + /pkgs/top-level/release-lib.nix) { inherit supportedSystems; };

  mkNova = pkgs: import src { inherit pkgs; repos = novaRepos; };
  novaFor = system: mkNova (releaseLib.pkgsFor system);
  novaForAllSystems = f: releaseLib.forAllSystems (system: f (novaFor system));
}
