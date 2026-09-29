{
  rosPackages = pkgs: with pkgs;
  let
    nixpkgs = import <nixpkgs> {};
    libuvc-theta = nixpkgs.callPackage ../../../nixfiles/packages/other/libuvc-theta { };
    gstthetauvc = nixpkgs.callPackage ../../../nixfiles/packages/other/gstthetauvc {
      inherit libuvc-theta;
      libusb = nixpkgs.libusb1;
    };
  in {
    nova-camera-msgs = callPackage ./camera_msgs { };

    nova-cameras = callPackage ./cameras3 {
      inherit libuvc-theta gstthetauvc;
    };
  };
}
