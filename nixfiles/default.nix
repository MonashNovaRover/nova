{ localSystem ? builtins.currentSystem
, crossSystem ? localSystem
, pkgs ? import <nixpkgs> { inherit localSystem crossSystem; }

  # The locations of checked-out Nova Rover repositories.
  # Each repository in this list should have a default.nix module file.
, repos ? import ./external/default-paths.nix
}:

let
  # Pin the version of Nixpkgs to ensure reproducibility.
  # Preferably, this will come from https://github.com/lopsided98/nixpkgs/tree/nix-ros,
  # but upstream Nixpkgs may be used if it a) has merged all pending PRs from
  # nix-ros and b) has newer changes we need.
  # Using the nix-ros branch with specific patches added is preferred because
  # it allows use of the https://ros.cachix.org binary cache.
  nixpkgs = pkgs.applyPatches {
    src = pkgs.fetchFromGitHub {
      owner = "NixOS";
      repo = "nixpkgs";
      rev = "2a9d660ff0f7ffde9d73be328ee6e6f10ef66b28";
      hash = "sha256-x3ZOPGLvtC0/+iFAg9Kvqm/8hTAIkGjc634SqtgaXTA=";
    };
    patches = [ ];
  };

  nix-ros-overlay = pkgs.applyPatches {
    src = pkgs.fetchFromGitHub {
      owner = "lopsided98";
      repo = "nix-ros-overlay";
      rev = "3205dadcff4e1fbc779ce3962bafcf3e9f3e931b";
      hash = "sha256-FUAvjcpVyDMZ2zCqUrpGlLtRZlF3PqW2alpyi82bMa8=";
    };
    patches = [
      # Make buildEnv more flexible
      (pkgs.fetchpatch {
        url = "https://github.com/lopsided98/nix-ros-overlay/pull/269.patch";
        hash = "sha256-9jxS4/5YskbxFhqnGoNsgXX21IK+YJWyyICsWCZZnfo=";
      })
      # Build ROS Python packages like colcon
      (pkgs.fetchpatch {
        url = "https://github.com/lopsided98/nix-ros-overlay/compare/3205dadcff4e1fbc779ce3962bafcf3e9f3e931b...6bdb199e0656650ad9d2e458be21a51d53c993e9.patch";
        hash = "sha256-ZJTeyVauQV+EJ5WX3FSlDAWt67e3p1oGIFDRCdFxAjk=";
      })
    ];
  };

  nix-ros-workspace = pkgs.fetchFromGitHub {
    owner = "hacker1024";
    repo = "nix-ros-workspace";
    rev = "6cb8d26ca240e5137b72c1071cc5d5b381e47e22";
    hash = "sha256-b7A9RvA156IEYBLdxhkM/af+pncWeFQkKLYzJ9e84YY=";
  };

  inherit (pkgs.lib.evalModules {
    modules = [
      (import ./external/out-of-tree.nix)
    ] ++ map import repos;
  }) config options;

  novaPkgs = import nixpkgs {
    localSystem = pkgs.buildPlatform;
    crossSystem = pkgs.hostPlatform;
    inherit (pkgs) config;

    overlays = [
      # Add the nix-ros-overlay. This supplies vanilla ROS packages.
      (import (nix-ros-overlay + /overlay.nix))

      # Add the nix-ros-workspace overlay. This adds more functionallity to nix-ros-overlay.
      (import nix-ros-workspace).overlay

      # Add the custom overlay. This:
      #  - Adds custom library functions
      #  - Applies patches to existing packages from Nixpkgs and the ROS overlay
      #  - Creates a "ros" alias pointing to "rosPackages.${version}"
      (import ./overlay)

      # Add internally defined packages.
      (self: super: import ./packages/other { inherit (self) callPackage; })
      (self: super: {
        pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
          (pyself: pysuper: import ./packages/python { inherit (pyself) callPackage; })
        ];
      })
      (self: super: {
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
in
rec {
  inherit repos config options;
  pkgs = novaPkgs;

  homeModule = {
    imports = [ ./home ];
    nixpkgs.overlays = [ (self: super: { nova = pkgs; }) ];
  };

  nixosModule = pkgs.lib.makeOverridable
    ({ homeManagerNixOSModule }: { ... }: {
      imports = [ homeManagerNixOSModule ./nixos ];
      nixpkgs.overlays = [ (self: super: { nova = pkgs; }) ];
      home-manager.nova.sharedModules = [ homeModule ];
    })
    { homeManagerNixOSModule = <home-manager/nixos>; };
}
