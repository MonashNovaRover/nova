{ pkgs, stdenv }:

let
  nixpkgs = pkgs.fetchFromGitHub {
    owner = "NixOS";
    repo = "nixpkgs";
    rev = "5b1bc788f578cd83d54b48bb057d6f6703ae7725";
    hash = "sha256-sDMEZ4HLP6sVNiBcgla3KWihdDjh67DP5ZWkGKWFgY0=";
  };
  rospkgs = pkgs.applyPatches {
    src = pkgs.fetchFromGitHub {
      owner = "lopsided98";
      repo = "nix-ros-overlay";
      inherit ((pkgs.lib.importJSON ../../../../../../nixfiles/revisions.json).nix-ros-overlay) rev hash;
    };
    patches = [
      # ament-cmake-vendor-package: don't 'fix' cmake files
      # https://github.com/lopsided98/nix-ros-overlay/pull/417
      #
      # This patch uses an overrideAttrs form that does not exist in this
      # revision of Nixpkgs.
      # Luckily, no dependencies of ours are affected by the bug this fixes.
      (pkgs.fetchpatch {
        url = "https://github.com/lopsided98/nix-ros-overlay/commit/9bc338678f854cec910c5d3b89663de1a75da222.patch";
        revert = true;
        hash = "sha256-TL6sGoby1rbLmloXxfYf+TrxLBVHETLp/aw6j/rahbM=";
      })
    ];
  };
in
import rospkgs {
  inherit (stdenv.buildPlatform) system;
  inherit nixpkgs;
  overlays = [
    # Include the top-level repo overlay.
    (import ../../../../../../nixfiles/overlay)

    # Add internally defined top-level repo packages.
    (self: super: import ../../../../../../nixfiles/packages/other { inherit (self) callPackage; })
    (self: super: {
      pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
        (pyself: pysuper: import ../../../../../../nixfiles/packages/python { inherit (pyself) callPackage; })
      ];
    })
    (self: super: {
      rosPackages = super.rosPackages.appendDistroOverlay
        (rosSelf: rosSuper: import ../../../../../../nixfiles/packages/ros { inherit (rosSelf) callPackage; })
        super.rosPackages;
    })

    # Nixpkgs compatibility fixes
    (self: super: {
      rosPackages = super.rosPackages // {
        jazzy = super.rosPackages.jazzy.overrideScope (rosSelf: rosSuper: {
          # Shim for future Nixpkgs attribute
          python = rosSuper.python // { pythonOnBuildForHost = rosSuper.python.pythonForBuild; };

          uncrustify-vendor = rosSelf.lib.patchAmentVendorGit rosSuper.uncrustify-vendor {
            url = "https://github.com/uncrustify/uncrustify.git";
            rev = "uncrustify-0.78.1";
            fetchgitArgs.hash = "sha256-L+YEVZC7sIDYuCM3xpSfZLjA3B8XsW5hi+zV2NEgXTs=";
          };
        });
      };
    })

    # Legacy packages
    (self: super: {
      # ROS packages
      ros = super.ros.overrideScope (rosSelf: rosSuper: {
        nova-camera-msgs = rosSelf.callPackage ../../packages/camera-msgs { };
        nova-cameras2 = rosSelf.callPackage ../../packages/cameras2 { };
      });

      # Python packages
      pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
        (pySelf: pySuper: {
          pygobject-stubs =
            pySelf.callPackage ./python-modules/pygobject-stubs { };
          pyqt5-stubs = pySelf.callPackage ./python-modules/pyqt5-stubs { };
        })
      ];

      # Runtime dependencies
      gst_all_1 = super.gst_all_1 // {
        gst-plugins-rs-webrtc = self.gst_all_1.gst-plugins-rs.override {
          # Using binary substitutions from Hydra are typically more convinent than building a minimal version of gst-plugins-rs.
          # The minimal version is 90% smaller, but takes a very long time to build.
          # plugins = [ "webrtc" "rtp" ];
        };
      };
      stunserver = self.callPackage ./stunserver { };
    })
  ];
}
