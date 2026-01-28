{
  stdenv,
  lib,
  fetchFromGitHub,
  meson,
  ninja,
  pkg-config,
  python3,
  gst_all_1,
  gettext,
  # Checks meson.is_cross_build(), so even canExecute isn't enough.
  directoryListingUpdater,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "gst-namedpipe";
  version = "0.0.1";

  outputs = [
    "out"
    "dev"
  ];

  src = fetchFromGitHub {
    owner = "aler9";
    repo = "gst-namedpipe";
    rev = "200dd1260c68673e0bdbeb3bce98d6d53a9732fa";
    hash = "sha256-TWwmeHgdoeuJXDnJj529TU19hKuf9ZurMHwfp9Z7sBQ=";
  };

  nativeBuildInputs = [
    meson
    ninja
    gettext
    pkg-config
    python3
  ];

  buildInputs = [
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
  ];

  postPatch = ''
    patchShebangs \
      scripts/extract-release-date-from-doap-file.py
  '';

  passthru = {
    updateScript = directoryListingUpdater { };
  };

  meta = {
    platforms = lib.platforms.unix;
  };
})
