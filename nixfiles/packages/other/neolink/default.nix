{ rustPlatform
, fetchFromGitHub
, openssl
, gst_all_1
, pkg-config
, glib
, gobject-introspection
, lib
}:
rustPlatform.buildRustPackage rec {
  pname = "neolink";
  version = "0.6.2";
  #cargoLock.lockFile = ./Cargo.lock;
  cargoHash = "sha256-BMEBDX5oE3nxFDauXwT65VN3RkMmIXltU+duBi+BXsA=";
  src = fetchFromGitHub {
    owner = "QuantumEntangledAndy";
    repo = "neolink";
    rev = "6e05e7844b5b50f89787d30bffcbbd3471bfcfde";
    hash = "sha256-/byGj3Gz+dcriPwyAN54Nppl/UQK2WMD8bYh74wy2t8=";
  };
  env = {
    #"GSTREAMER_1.0_NO_PKG_CONFIG" = "1";
    #"GLIB_2.0_NO_PKG_CONFIG" = "1";
    #"GSTREAMER_SDP_1.0_NO_PKG_CONFIG" = "1";
    PKG_CONFIG_PATH = lib.strings.concatMapStringsSep ":" (x: "${x.dev}/lib/pkgconfig")
                [
                  gst_all_1.gstreamer
                  gst_all_1.gst-plugins-base
                  gst_all_1.gst-plugins-good
                  gst_all_1.gst-plugins-bad
                  gst_all_1.gst-rtsp-server
                  glib
                 gobject-introspection
                ];
  };

  nativeBuildInputs = [
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
    gst_all_1.gst-plugins-good
    gst_all_1.gst-plugins-bad
    gst_all_1.gst-rtsp-server
    glib
    gobject-introspection
  #libgstreamer1.0-0 \
  #libgstreamer-plugins-bad1.0-0 \
  openssl
  pkg-config

  ];
}
