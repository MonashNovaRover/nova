{ pkgs ? import <nixpkgs> { }

  # The locations of checked-out Nova Rover repositories.
  # Each repository in this list should have a default.nix module file.
, repos ? import ./external/default-paths.nix
}:

let
  revisions = builtins.fromJSON (builtins.readFile ./revisions.json);

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
      inherit (revisions.nixpkgs) rev hash;
    };
    patches = [
      # python3.pkgs.pygobject-stubs: init at 2.8.0
      (pkgs.fetchpatch {
        url = "https://github.com/NixOS/nixpkgs/pull/249663/commits/83326c2d47189cbe098c0719c56408f93c1e61d7.patch";
        hash = "sha256-qRjwkTbqLGG7/tvW1tXZZd2Tj88rmFkmoUNxgUs+yC0=";
      })
    ];
  };

  nix-ros-overlay = pkgs.applyPatches {
    src = pkgs.fetchFromGitHub {
      owner = "lopsided98";
      repo = "nix-ros-overlay";
      inherit (revisions.nix-ros-overlay) rev hash;
    };
    patches = [
      # Bring back Foxy
      (pkgs.fetchpatch {
        url = "https://github.com/lopsided98/nix-ros-overlay/commit/113129c9daf2edfcb4a12593a60e918116dee671.patch";
        revert = true;
        hash = "sha256-73+rJj9c34cHQkDqxHzSe0HDAf+3zMJj4g+04y25xAU=";
      })
    ];
  };

  nix-ros-workspace = pkgs.fetchFromGitHub {
    owner = "hacker1024";
    repo = "nix-ros-workspace";
    inherit (revisions.nix-ros-workspace) rev hash;
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
