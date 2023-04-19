{ pkgs ? (import ./. { }).pkgs }:

pkgs.mkShell {
  packages = with pkgs; [
    gst-plugin-webrtc-signalling
    stunserver
  ];

  inputsFrom = [
    (pkgs.ros.buildEnv {
      paths = [
        pkgs.ros.rmw-fastrtps-dynamic-cpp
        pkgs.ros.python.pkgs.pygobject-stubs
        pkgs.ros.python.pkgs.pyqt5-stubs
      ];
    }).env
    pkgs.ros.cameras2
  ];

  RMW_IMPLEMENTATION = "rmw_fastrtps_dynamic_cpp"; # https://github.com/lopsided98/nix-ros-overlay/issues/45
}
