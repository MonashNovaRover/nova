{ nixpkgs ? import <nixpkgs> { } }:

rec {
  pkgs = nixpkgs.callPackage ./packages { };
  launcher = pkgs.callRosPackage ./launcher { } { };
}
