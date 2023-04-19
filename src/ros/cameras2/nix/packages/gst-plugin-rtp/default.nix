{ fetchFromGitLab
, rustPlatform
, pkg-config
, glib
, gst_all_1
}:

rustPlatform.buildRustPackage rec {
  pname = "gst-plugin-rtp";
  version = "0.10.2";

  src = fetchFromGitLab {
    domain = "gitlab.freedesktop.org";
    owner = "gstreamer";
    repo = "gst-plugins-rs";
    rev = version;
    hash = "sha256-bMqun2uFd32sG2wKT9WrACy5Qxu5jKndcUUl997Di7o=";
  };

  buildAndTestSubdir = "net/rtp";

  cargoHash = "sha256-HMEJHyqT6xsEx+NT/8H7atRvO0q5QcrPt3Ta04OZqHk=";

  nativeBuildInputs = [ pkg-config ];
  buildInputs = [
    glib
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
  ];

  postInstall = ''
    mkdir $out/lib/gstreamer-1.0
    mv $out/lib/*.so $out/lib/gstreamer-1.0
  '';
}
