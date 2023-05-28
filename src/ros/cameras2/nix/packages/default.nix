{ pkgs, stdenv }:

let
  nixpkgs = pkgs.fetchFromGitHub {
    owner = "NixOS";
    repo = "nixpkgs";
    rev = "aeb75dba965e790de427b73315d5addf91a54955";
    hash = "sha256-U3oOge4cHnav8OLGdRVhL45xoRj4Ppd+It6nPC9nNIU=";
  };
  rospkgs = pkgs.fetchFromGitHub {
    owner = "lopsided98";
    repo = "nix-ros-overlay";
    rev = "650b5972c994c6d13fdac70869c19c52c056d8de";
    hash = "sha256-czIXVpxwnsqAmHMj+XJdcqiwvmGoq9nPuWCkeZ0sLik=";
  };
in import rospkgs {
  inherit (stdenv.buildPlatform) system;
  inherit nixpkgs;
  overlays = [
    (self: super: {
      ros = self.rosPackages.foxy;
      callRosPackage = path: overrides: rosOverrides:
        self.ros.callPackage (self.callPackage path overrides) rosOverrides;
    })
    (self: super: {
      # ROS packages
      ros = super.ros.overrideScope (rosSelf: rosSuper: {
        camera-msgs = self.callRosPackage ./camera-msgs { } { };
        cameras2 = self.callRosPackage ./cameras2 { } { };
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
