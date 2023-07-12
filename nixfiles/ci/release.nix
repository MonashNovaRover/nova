{ supportedSystems ? [ "x86_64-linux" "aarch64-linux" ]
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

  novaFor = system:
    # Based on https://github.com/NixOS/nixpkgs/blob/bec27fabee7ff51a4788840479b1730ed1b64427/pkgs/top-level/release-lib.nix#L24C21-L24C21.
    # Caches instances to avoid repeated evaluations.
    let
      nova_x86_64_linux = mkNova (lib.pkgsFor "x86_64-linux");
      nova_aarch64_linux = mkNova (lib.pkgsFor "aarch64-linux");
    in
    if system == "x86_64-linux" then nova_x86_64_linux
    else if system == "aarch64-linux" then nova_aarch64_linux
    else abort "unsupported system type: ${system}";

  genNovaPlatformRecipes = f: lib.forAllSystems (system: f (novaFor system));
in
builtins.mapAttrs (name: genNovaPlatformRecipes) rec {
  workspace = nova: nova.pkgs.ros.nova-workspace;
  workspace-env-inputs = nova: (workspace nova).env.inputDerivation;
}
