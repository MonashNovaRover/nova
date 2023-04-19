{ fetchFromGitLab
, rustPlatform
, pkg-config
, openssl
, glib
, gst_all_1
}:

rustPlatform.buildRustPackage rec {
  pname = "gst-plugin-webrtc";
  version = "0.10.2";

  src = fetchFromGitLab {
    domain = "gitlab.freedesktop.org";
    owner = "gstreamer";
    repo = "gst-plugins-rs";
    rev = version;
    hash = "sha256-bMqun2uFd32sG2wKT9WrACy5Qxu5jKndcUUl997Di7o=";
  };

  buildAndTestSubdir = "net/webrtc";

  cargoHash = "sha256-TdzK6anh2pnIdx7bfSE43zI3gh6iQAgi3qWEIS96J/4=";

  nativeBuildInputs = [ pkg-config ];
  buildInputs = [
    openssl
    glib
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
    gst_all_1.gst-plugins-bad
  ];

  postInstall = ''
    mkdir $out/lib/gstreamer-1.0
    mv $out/lib/*.so $out/lib/gstreamer-1.0
  '';
}
