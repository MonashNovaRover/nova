{ pkgs ? import <nixpkgs> {} }:

let
  unstable = import <nixos-unstable> {
    inherit (pkgs) system;
  };
in

pkgs.mkShell {
  buildInputs = with unstable.gst_all_1; [
    gstreamer
    gst-plugins-base
    gst-plugins-good
    gst-plugins-bad
    gst-plugins-ugly
    gst-libav
    gst-plugins-rs
  ] ++ [
    unstable.svt-av1
    unstable.libnice
    unstable.v4l-utils
  ];
}
