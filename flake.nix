{
  description = "A very basic flake";

  inputs = {
    nixpkgs = {
      url = "github:nixos/nixpkgs/cb9e5af795f0307a41dfe32748f025a050d9fe30";
    };
    nix-ros-overlay = {
      url = "github:lopsided98/nix-ros-overlay/4072d6ed51d9053d2cc85c0ec4f69884cc99f392";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    nix-ros-workspace = {
      url = "github:hacker1024/nix-ros-workspace/e341a5fd27af73be15bc0396c5745c54398193aa";
      flake = false;
    }; 
    teleop-modular = {
      url = "github:baileychessum/teleop_modular/0ccb9e3aa3d8258b59e9ccc31a8bb68d4980f296";
      flake = false;
    };
    jetpack-nixos = {
      url = "github:anduril/jetpack-nixos/4d857d6da48c420da39964b6fb7ba23a958abad3";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    self = {
      submodules = true;
    };
  };


  outputs = { nixpkgs, nix-ros-overlay, nix-ros-workspace, teleop-modular, jetpack-nixos, self, self-clean }@inputs: let
    inherit (nixpkgs.lib.evalModules {
      modules = [
        (import nixfiles/external/out-of-tree.nix)
        (import ./src)
      ];
    }) config options;

    systems = [ "x86_64-linux" "aarch64-linux" ];
    forAllSystems = f: builtins.listToAttrs (map (system: {
        name = system;
        value = f system;
      }) systems);

  in {
    checks = {}; # import ./tests { hostPkgs = pkgs; inherit novaPkgs};

    nixosModules.default = import nixfiles/modules/nixos;

      #throw (builtins.removeAttrs self [ "outPath" "_type" "a" "checks" "dirtyRev" "inputs" "outputs" "overlays" "packages" "nixosModules" "narHash"
  #"dirtyShortRev" "lastModified" "lastModifiedDate"]);
    overlays.default = final: prev: nixpkgs.lib.composeManyExtensions [
      # Add the nix-ros-overlay. This supplies vanilla ROS packages.
      nix-ros-overlay.overlays.default

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
      (import nixfiles/overlay)

      # Add teleop_modular
      (import (teleop-modular + "/overlay.nix"))

      # Add internally defined packages.
      (self: super: import nixfiles/packages/other { inherit (self) pkgs callPackage; })
      (self: super: {
        pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
          (pyself: pysuper: import nixfiles/packages/python { inherit (pyself) callPackage; })
        ];
      })
      (self: super: {
        rosPackages = super.rosPackages.appendDistroOverlay
        (rosSelf: rosSuper: import nixfiles/packages/ros {
          inherit (rosSelf) callPackage;
          pkgs = rosSelf;
          git-metadata = builtins.readFile (
            self.pkgs.writers.writeJSON "metadata.json" (
                # if outPath is in the attrset, it just prints the store path not the rest of the set.
                (builtins.removeAttrs inputs.self.sourceInfo [ "outPath" ]) // { source = inputs.self.sourceInfo.outPath; }
              )
            );
        })
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
      #(self: super: { nova = result; })
    ]
      final
      prev;



    packages = forAllSystems (system: let
      pkgs = import nixpkgs {
        inherit system;

        config = {
          permittedInsecurePackages = [
            "freeimage-unstable-2021-11-01"
            "freeimage-3.18.0-unstable-2024-04-18"
          ];
        };

        overlays = [
          self.overlays.default
        ];
      };
    in pkgs // {
      default = pkgs.ros.nova-workspace;
    });


  };
}
