{ pkgs ? import <nixpkgs> { }

  # The locations of checked-out Nova Rover repositories.
  # See the default value for the structure.
, repos ? import ./external/default-paths.nix
}:

let
  # Pin the version of Nixpkgs to ensure reproducibility.
  # Use the fork made for ROS.
  nixpkgs = pkgs.fetchFromGitHub {
    owner = "lopsided98";
    repo = "nixpkgs";
    rev = "f68a0d0fd7539d93c7454989f71fd1c824f3b46f";
    hash = "sha256-oJhMiKnJeb47gTxyyIgAJf+aWC4IbVmG3wgL3ZVJ0Eg=";
  };

  nix-ros-overlay = pkgs.applyPatches {
    src = pkgs.fetchFromGitHub {
      owner = "lopsided98";
      repo = "nix-ros-overlay";
      rev = "381164fc34d6b9d8b59f4e45e19cad1bec709892";
      hash = "sha256-6RBNIfxJFMMKaOfSG/h3w0zWTQU/cAHbDRnhI7vMLFw=";
    };
    patches = [
      # Make buildEnv more flexible
      (pkgs.fetchpatch {
        url = "https://github.com/lopsided98/nix-ros-overlay/pull/269.patch";
        hash = "sha256-9jxS4/5YskbxFhqnGoNsgXX21IK+YJWyyICsWCZZnfo=";
      })
    ];
  };

  external = import ./external/out-of-tree.nix repos;
in
{
  pkgs = import nixpkgs {
    overlays = [
      # Add the nix-ros-overlay. This supplies vanilla ROS packages.
      (import (nix-ros-overlay + /overlay.nix))

      # Add the custom overlay. This:
      #  - Adds custom library functions
      #  - Applies patches to existing packages from Nixpkgs and the ROS overlay
      #  - Creates a "ros" alias pointing to "rosPackages.${version}"
      (import ./overlay)

      # Add internally defined packages and scopes.
      (self: super:
        import ./packages { inherit (self) callPackage; } // {
          ros = super.ros.overrideScope (rosSelf: rosSuper:
            import ./packages/ros { inherit (rosSelf) callPackage; } // {
              # Add ROS packages to a "nova" scope to avoid naming colisions
              # with public ROS packages.
              nova = rosSelf.lib.makeScope rosSelf.newScope (nova:
                import ./packages/ros/nova { inherit (nova) callPackage; }
              );
            });
        })

      # Add externally defined (out-of-tree) Nova Rover packages.
      (self: super: import ./packages { inherit (self) callPackage; } // {
        # Add non-ROS Python packages.
        pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
          (pyself: pysuper:
            builtins.mapAttrs
              (name: directory: pyself.callPackage directory { })
              external.pythonPackages)
        ];

        # Add ROS packages.
        ros = super.ros.overrideScope (rosSelf: rosSuper: {
          nova = rosSuper.nova.overrideScope' (novaSelf: novaSuper:
            builtins.mapAttrs
              (name: directory: novaSelf.callPackage directory { })
              external.rosPackages);
        });
      })
    ];
  };
}
