{ lib
, fetchFromGitHub
, applyPatches
}:

let
  flakes-compat = fetchFromGitHub {
    owner = "NixOS";
    repo = "flake-compat";
    rev = "5edf11c44bc78a0d334f6334cdaf7d60d732daab";
    hash = "sha256-vNpUSpF5Nuw8xvDLj2KCwwksIbjua2LZCqhV1LNRDns=";
  };
  jetpack-nixos  = applyPatches {
    src = fetchFromGitHub {
          owner = "anduril";
          repo = "jetpack-nixos";
          rev = "79a0ba1d5df6bfef19b425169fcb8478ecf2686f";
          hash = "sha256-SiBHFVVmyCZbZFqCN+tIqbpRDxBErq0fScWJQnr93PM=";
        };
    patches = [
      ./0002-flash-for-novacarrier.patch
      ./0003-nar-hash.patch
    ];
  };
in
(import ( flakes-compat
) { src = jetpack-nixos; }).defaultNix.outputs.packages.x86_64-linux.flash-orin-nano-novacarrier
