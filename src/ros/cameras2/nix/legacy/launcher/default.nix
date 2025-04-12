{ lib
, writeShellScriptBin
, stunserver
, gst_all_1
, buildEnv
, ros2launch
, nova-cameras2
}:

let
  packages = [
    (buildEnv { paths = [ ros2launch nova-cameras2 ]; })
    stunserver
    gst_all_1.gst-plugins-rs-webrtc # For the signalling server
  ];
in
writeShellScriptBin "gst-nova-launcher" ''
  export PATH="${lib.makeBinPath packages}:$PATH"

  if [ -z "''${1-}" ]; then
    echo >&2 "Usage: $(basename "$0") command-to-run args..."
    exit 1
  fi

  exec -- "$1" "''${@:2}"
''
