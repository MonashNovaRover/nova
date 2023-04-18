{ fetchFromGitLab
, rustPlatform
, pkg-config
, openssl
}:

rustPlatform.buildRustPackage rec {
  pname = "gst-plugin-webrtc-signalling";
  version = "0.10.2";

  src = fetchFromGitLab {
    domain = "gitlab.freedesktop.org";
    owner = "gstreamer";
    repo = "gst-plugins-rs";
    rev = version;
    hash = "sha256-bMqun2uFd32sG2wKT9WrACy5Qxu5jKndcUUl997Di7o=";
  };

  buildAndTestSubdir = "net/webrtc/signalling";

  cargoHash = "sha256-aEqZO3Zm0cyZ6vPCK5t79nkWZfG5VvQdpxqqGtzKmgs=";

  nativeBuildInputs = [ pkg-config ];
  buildInputs = [ openssl ];
}
