{ supportedSystems
, nixpkgs
, nova-monorepo
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

  mkNova = pkgs: import nixfiles { inherit pkgs; };
  novaFor = system: mkNova (releaseLib.pkgsFor system);
  novaForAllSystems = f: releaseLib.forAllSystems (system: f (novaFor system));
}
