{ pkgs ? import <nixpkgs> { }

  # The locations of checked-out Nova Rover repositories.
  # Each repository in this list should have a default.nix module file.
, repos ? import ./external/default-paths.nix
}:

let
  revisions = builtins.fromJSON (builtins.readFile ./revisions.json);

  maybeApplyPatches = { src, patches, ... }@args: if patches == [ ] then src else pkgs.applyPatches args;

  # Pin the version of Nixpkgs to ensure reproducibility.
  # Preferably, this will come from https://github.com/lopsided98/nixpkgs/tree/nix-ros,
  # but upstream Nixpkgs may be used if it a) has merged all pending PRs from
  # nix-ros and b) has newer changes we need.
  # Using the nix-ros branch with specific patches added is preferred because
  # it allows use of the https://ros.cachix.org binary cache.
  nixpkgs = pkgs.lib.maybeEnv "NIXPKGS_PATH" (maybeApplyPatches {
    src = pkgs.fetchFromGitHub {
      owner = "NixOS";
      repo = "nixpkgs";
      inherit (revisions.nixpkgs) rev hash;
    };
    patches = [
    ];
  });

  # nix-ros-overlay = ../nix-ros-overlay;
  nix-ros-overlay = pkgs.lib.maybeEnv "NRO_PATH" (maybeApplyPatches {
    src = pkgs.fetchFromGitHub {
      owner = "lopsided98";
      repo = "nix-ros-overlay";
      inherit (revisions.nix-ros-overlay) rev hash;
    };
    patches = [
      # # fix: gz vendor
      # # https://github.com/lopsided98/nix-ros-overlay/pull/472
      # ./overlay/ros/patches/nix-ros-workspace.patch

      # # Some more Gazebo improvements
      # # https://github.com/muellerbernd/nix-ros-overlay/pull/2
      # (pkgs.fetchpatch {
      #   url = "https://github.com/lopsided98/nix-ros-overlay/compare/6d04148eac0727be34e5333f6e12cfc7e86673c3...eca9687ce15335bbb2d4b7b14fbf74ce0e957f43.patch";
      #   hash = "sha256-c6DD2U6Lo2dcs0APxEHg9l0bz1Ioa5aX5FoATajXYAc=";
      # })
    ];
  });

  nix-ros-workspace = pkgs.lib.maybeEnv "NRWS_PATH" (pkgs.fetchFromGitHub {
    owner = "hacker1024";
    repo = "nix-ros-workspace";
    inherit (revisions.nix-ros-workspace) rev hash;
  });

  inherit (pkgs.lib.evalModules {
    modules = [
      (import ./external/out-of-tree.nix)
    ] ++ map import repos;
  }) config options;

  # Extend Nixpkgs with custom packages.
  novaPkgs = import nixpkgs {
    localSystem = pkgs.buildPlatform.system;
    crossSystem = pkgs.hostPlatform.system;
    config = pkgs.config // {
      permittedInsecurePackages = pkgs.config.permittedInsecurePackages or [ ] ++ [
        "freeimage-unstable-2021-11-01"
        "freeimage-3.18.0-unstable-2024-04-18"
      ];
    };

    overlays = [
      # Add the nix-ros-overlay. This supplies vanilla ROS packages.
      (import (nix-ros-overlay + /overlay.nix))

      # Add the nix-ros-overlay FOD as a package.
      # This, much like Nixpkgs's path attribute, allows callers to access files
      # from the project.
      (self: super: { inherit nix-ros-overlay; })

      # Add the nix-ros-workspace overlay. This adds more functionallity to nix-ros-overlay.
      (import nix-ros-workspace { }).overlay

      # Add the custom overlay. This:
      #  - Adds custom library functions
      #  - Applies patches to existing packages from Nixpkgs and the ROS overlay
      #  - Creates a "ros" alias pointing to "rosPackages.${version}"
      (import ./overlay)

      # Add teleop_modular
      # To use local source code for teleop modular, run in your terminal before building:
      # $ export TELEOP_PATH=~/path/to/teleop_modular
      # (this only applies to the one terminal, and wont apply to any future shell sessions)
      (import ((pkgs.lib.maybeEnv "TELEOP_PATH" (pkgs.fetchFromGitHub {
        owner = "BaileyChessum";
        repo = "teleop_modular";
        inherit (revisions.teleop-modular) rev hash;
      })) + "/overlay.nix"))

      # Add internally defined packages.
      (self: super: import ./packages/other { inherit (self) pkgs callPackage; })
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

    tests = import ./tests {
      hostPkgs = pkgs;
      inherit novaPkgs;
    };

    # Make an alias to the nova-workspace environment for convenience.
    inherit (result.pkgs.ros.nova-workspace) env;

    # Random things that don't fit anywhere else.
    misc = {
      cameras2-legacy = import ../src/ros/cameras2/nix/legacy/default.nix { nixpkgs = pkgs; };
    };
  };
in
result
