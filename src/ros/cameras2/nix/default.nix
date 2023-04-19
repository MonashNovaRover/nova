{ nixpkgs ? import <nixpkgs> { } }:

rec {
  pkgs = nixpkgs.callPackage ./packages { };
  launcher = pkgs.callRosPackage ./launcher { } { };
  env = pkgs.callPackage ./shell.nix { };
}
