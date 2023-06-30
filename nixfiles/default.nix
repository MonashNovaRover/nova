{ pkgs ? import <nixpkgs> { }

  # The locations of checked-out Nova Rover repositories.
  # Each repository in this list should have a default.nix module file.
, repos ? import ./external/default-paths.nix
}:

let
  # Pin the version of Nixpkgs to ensure reproducibility.
  # Use the fork made for ROS.
  nixpkgs = pkgs.applyPatches {
    src = pkgs.fetchFromGitHub {
      owner = "lopsided98";
      repo = "nixpkgs";
      rev = "f68a0d0fd7539d93c7454989f71fd1c824f3b46f";
      hash = "sha256-oJhMiKnJeb47gTxyyIgAJf+aWC4IbVmG3wgL3ZVJ0Eg=";
    };
    patches = [
      # prefetch-npm-deps: add retry
      (pkgs.fetchpatch {
        url = "https://github.com/NixOS/nixpkgs/pull/238452.patch";
        hash = "sha256-15sPCCdpirn4u6NflqZ0cknT/N96gd3RLXZVxBG4LWw=";
      })
      # prefetch-npm-deps: use isahc instead of ureq
      (pkgs.fetchpatch {
        url = "https://github.com/NixOS/nixpkgs/pull/240419.patch";
        hash = "sha256-utwcc06OdsHh4/ZL9nwY0AedH8nmWya7aEMUuFhlo5w=";
      })
    ];
  };

  nix-ros-overlay = pkgs.applyPatches {
    src = pkgs.fetchFromGitHub {
      owner = "lopsided98";
      repo = "nix-ros-overlay";
      rev = "6b6482e0b4cb197f77ef2cacdb69c5ba233c44b1";
      hash = "sha256-RA2mdhz7MM10MuAYUDmkJg3cZucize6IluZfcLwpBvM=";
    };
    patches = [
      # Make buildEnv more flexible
      (pkgs.fetchpatch {
        url = "https://github.com/lopsided98/nix-ros-overlay/pull/269.patch";
        hash = "sha256-9jxS4/5YskbxFhqnGoNsgXX21IK+YJWyyICsWCZZnfo=";
      })
    ];
  };

  config = (pkgs.lib.evalModules {
    modules = [
      (import ./external/out-of-tree.nix)
    ] ++ map import repos;
  }).config;
in
{
  inherit repos config;
  pkgs = import nixpkgs {
    overlays = [
      # Add the nix-ros-overlay. This supplies vanilla ROS packages.
      (import (nix-ros-overlay + /overlay.nix))

      # Add the custom overlay. This:
      #  - Adds custom library functions
      #  - Applies patches to existing packages from Nixpkgs and the ROS overlay
      #  - Creates a "ros" alias pointing to "rosPackages.${version}"
      (import ./overlay)

      # Add internally defined packages.
      (self: super:
        import ./packages { inherit (self) callPackage; } // {
          pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
            (pyself: pysuper: import ./packages/python { inherit (pyself) callPackage; })
          ];
          rosPackages = super.rosPackages.appendDistroOverlay
            (rosSelf: rosSuper: import ./packages/ros { inherit (rosSelf) callPackage; })
            super.rosPackages;
        })

      # Add externally defined (out-of-tree) packages.
      (self: super: config.packages self)
      (self: super: {
        pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
          (pyself: pysuper: config.pythonPackages pyself)
        ];
      })
      (self: super: {
        rosPackages = super.rosPackages.appendDistroOverlay
          (rosSelf: rosSuper: config.rosPackages rosSelf)
          super.rosPackages;
      })
    ];
  };
}
