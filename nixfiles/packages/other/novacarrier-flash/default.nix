{ lib
, fetchFromGitHub
, applyPatches
}:

let
  jetpack-nixos  = applyPatches {
    src = fetchFromGitHub {
          owner = "anduril";
          repo = "jetpack-nixos";
          rev = "79a0ba1d5df6bfef19b425169fcb8478ecf2686f";
          hash = "sha256-SiBHFVVmyCZbZFqCN+tIqbpRDxBErq0fScWJQnr93PM=";
        };
    patches = [
      ./0001-flake-compat.patch
      ./0002-flash-for-novacarrier.patch
      ./0003-nar-hash.patch
    ];
  };
in
(import jetpack-nixos ).outputs.packages.x86_64-linux.flash-orin-nano-novacarrier
