{ stdenv
, fetchFromGitHub
, pkg-config
, gst_all_1
, libuvc-theta
, libusb
, libglvnd
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "gstthetauvc";
  version = "2914a3ef2feb39a182a8cc505cfae0e2d8922f0a";

  src = fetchFromGitHub {
    owner = "nickel110";
    repo = "gstthetauvc";
    rev = finalAttrs.version;
    hash = "sha256-oWbi2oz/ErOZXPNTjiPmlVRo/XGFVteZou0K3SObdM0=";
  };

  sourceRoot = "source/thetauvc";

  nativeBuildInputs = [ pkg-config ];

  buildInputs = [
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
    libuvc-theta
    libusb
    libglvnd
  ];

  makeFlags = [ "WITH_TRANSFORM_FILTER=1" ];

  installPhase = ''
    install -Dm644 gstthetauvc.so "$out/lib/gstreamer-1.0/gstthetauvc.so"
  '';
})
