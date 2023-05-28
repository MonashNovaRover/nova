{ nixpkgs ? import <nixpkgs> { } }:

rec {
  pkgs = nixpkgs.callPackage ./packages { };
  launcher = pkgs.ros.callPackage ./launcher { };
  env = pkgs.callPackage ./shell.nix { };
}
