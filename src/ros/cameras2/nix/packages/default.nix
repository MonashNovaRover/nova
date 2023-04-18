{ pkgs }:

pkgs.callPackage ./ros.nix { } {
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
