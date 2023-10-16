{ stdenv
, fetchFromGitHub
, pkg-config
, gst_all_1
, libuvc-theta
, libusb
, libGL
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "gstthetauvc";
  version = "0c2bd3dbd2c061941796e423e1c84debd621867e";

  src = fetchFromGitHub {
    owner = "nickel110";
    repo = "gstthetauvc";
    rev = finalAttrs.version;
    hash = "sha256-Zuy6e2M1d9r/vzT8H2G83aMka32Pcbx3ngB4ZHxoGAE=";
  };

  sourceRoot = "source/thetauvc";

  nativeBuildInputs = [ pkg-config ];

  buildInputs = [
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
    libuvc-theta
    libusb
    libGL
  ];

  makeFlags = [ "WITH_TRANSFORM_FILTER=1" ];

  installPhase = ''
    install -Dm644 gstthetauvc.so "$out/lib/gstreamer-1.0/gstthetauvc.so"
  '';
})
