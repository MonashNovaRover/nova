{ pkgs ? import <nixpkgs> { }

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
      rev = "3d958404528cd939451ca2ed30473c3d7ae4d746";
      hash = "sha256-QZUxjv/MsWjradxgHlQFkP1ynR4BAuedY/Hs+gMyss8=";
    };
    patches = [
      # protobuf3: 24.4 -> 21.12
      # Newer versions of protobuf have issues releated to abseil-cpp.
      (pkgs.fetchpatch {
        url = "https://github.com/NixOS/nixpkgs/commit/9e397b9d0042839420047a4154e6e33fd634a846.patch";
        revert = true;
        hash = "sha256-npLoOB8eOLcJFgkoARzH9GfU5OUKJjfUf5c2l2IAzWk=";
      })
    ];
  };

  nix-ros-overlay = pkgs.applyPatches {
    src = pkgs.fetchFromGitHub {
      owner = "lopsided98";
      repo = "nix-ros-overlay";
      rev = "4c0baef357f0d0c84502202d75a69c496feb433e";
      hash = "sha256-PoL3GNYeexLNGTNHm3qkAv25aL4pbsMKepW7CT9Z1c8=";
    };
    patches = [
      # Bring back Foxy
      (pkgs.fetchpatch {
        url = "https://github.com/lopsided98/nix-ros-overlay/commit/113129c9daf2edfcb4a12593a60e918116dee671.patch";
        revert = true;
        hash = "sha256-73+rJj9c34cHQkDqxHzSe0HDAf+3zMJj4g+04y25xAU=";
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
    rev = "a79bac0a7c984f90e7924c378f9d93917dfed92f";
    hash = "sha256-Hj65PG6ykCHd51CWL1HGLX96HfEuxEv+8HQJVe0AjzI=";
  };

  inherit (pkgs.lib.evalModules {
    modules = [
      (import ./external/out-of-tree.nix)
    ] ++ map import repos;
  }) config options;

  # Extend Nixpkgs with custom packages.
  novaPkgs = import nixpkgs {
    localSystem = pkgs.buildPlatform;
    crossSystem = pkgs.hostPlatform;
    inherit (pkgs) config;

    overlays = [
      # Add the nix-ros-overlay. This supplies vanilla ROS packages.
      (import (nix-ros-overlay + /overlay.nix))

      # Add the nix-ros-overlay FOD as a package.
      # This, much like Nixpkgs's path attribute, allows callers to access files
      # from the project.
      (self: super: { inherit nix-ros-overlay; })

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

      # Add the return value of this function. Some other attributes are useful
      # when  only pkgs is available.
      (self: super: { nova = result; })
    ];
  };

  result = {
    inherit repos config options;

    # Inputs required for the evaluation of expressions in this repository.
    # It is useful to keep track of these, because Nix has no built-in way to do
    # so, and they are often accidentally garbage collected.
    inputs = {
      inherit nixpkgs nix-ros-overlay nix-ros-workspace;
      inherit (novaPkgs) github-gitignore;
    };

    pkgs = novaPkgs;

    # Make an alias to the nova-workspace environment for convenience.
    inherit (result.pkgs.ros.nova-workspace) env;
  };
in
result
