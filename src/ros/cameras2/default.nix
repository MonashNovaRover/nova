{ pkgs, rosPkgs }:
let
  callRosPackage = path: overrides: rosOverrides: rosPkgs.callPackage (pkgs.callPackage path overrides) rosOverrides;
in
{
  camera-msgs = callRosPackage ./nix/packages/camera-msgs { } { };
  cameras2 = callRosPackage ./nix/packages/cameras2 { } { };
}
