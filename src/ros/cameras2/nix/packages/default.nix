{ pkgs, stdenv }:

pkgs.callPackage ./ros.nix { } {
  inherit (stdenv.buildPlatform) system;
  nixpkgs = (pkgs.applyPatches {
    src = pkgs.fetchFromGitHub {
      owner = "NixOS";
      repo = "nixpkgs";
      rev = "da45bf6ec7bbcc5d1e14d3795c025199f28e0de0";
      hash = "sha256-sASwo8gBt7JDnOOstnps90K1wxmVfyhsTPPNTGBPjjg=";
    };
    patches = [
      (pkgs.fetchpatch {
        # gst-plugins-rs 0.10.6
        # Can be removed once https://nixpk.gs/pr-tracker.html?pr=225143 hits nixos-unstable
        url = "https://github.com/NixOS/nixpkgs/commit/7c79d2edc3f404eb836aaf7786db777f1e0cb144.patch";
        hash = "sha256-yiHCUy1MzPDMd9nWn5e72JjtRpfT6jczfOg8+I/w8Gg=";
      })
    ];
  });
  overlays = [
    (self: super: {
      ros = self.rosPackages.foxy;
      callRosPackage = path: overrides: rosOverrides: self.ros.callPackage (self.callPackage path overrides) rosOverrides;
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
          pygobject-stubs = pySelf.callPackage ./python-modules/pygobject-stubs { };
          pyqt5-stubs = pySelf.callPackage ./python-modules/pyqt5-stubs { };
        })
      ];

      # Runtime dependencies
      gst-plugin-webrtc = self.callPackage ./gst-plugin-webrtc { };
      gst-plugin-rtp = self.callPackage ./gst-plugin-rtp { };

      # External tools
      gst-plugin-webrtc-signalling = self.callPackage ./gst-plugin-webrtc-signalling { };
      stunserver = self.callPackage ./stunserver { };
    })
  ];
}
