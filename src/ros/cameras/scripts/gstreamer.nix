{ pkgs ? import <nixpkgs> {} }:

let
  unstable = import <nixos-unstable> {
    inherit (pkgs) system;
  };

  libuvc-theta = pkgs.callPackage /home/nova/nova/nixfiles/packages/other/libuvc-theta {};

  gstthetauvc = pkgs.callPackage /home/nova/nova/nixfiles/packages/other/gstthetauvc {
    inherit libuvc-theta;
    libusb = pkgs.libusb1;
  };
in

pkgs.mkShell {
  packages = with pkgs.gst_all_1; [
    gstreamer
    gst-plugins-base
    gst-plugins-good
    gst-plugins-bad
    gst-plugins-ugly
    gst-libav
    gst-plugins-rs
  ] ++ [
    pkgs.svt-av1
    pkgs.libnice
    pkgs.v4l-utils
  ];

  shellHook = ''
    export GST_PLUGIN_PATH="${gstthetauvc}/lib/gstreamer-1.0:$GST_PLUGIN_PATH"
  '';
}
