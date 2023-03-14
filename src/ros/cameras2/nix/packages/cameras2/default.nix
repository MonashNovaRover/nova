{ cmake, wrapGAppsHook, gobject-introspection, gst_all_1, gst-plugin-webrtc, gst-plugin-rtp, libnice }:
{ buildRosPackage, rclpy, std-srvs, python3Packages }:

buildRosPackage {
  pname = "ros-cameras2";
  version = "git";
  buildType = "ament_python";

  src = ../../../cameras2;

  nativeBuildInputs = [
    wrapGAppsHook
    gobject-introspection
    gst_all_1.gstreamer
  ];

  buildInputs = [
    gobject-introspection
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
    gst_all_1.gst-plugins-good # V4L2
    gst_all_1.gst-plugins-bad # WebRTC
    gst_all_1.gst-plugins-ugly # H264
    gst-plugin-webrtc # WebRTC
    gst-plugin-rtp # WebRTC
    libnice # WebRTC
  ];

  propagatedBuildInputs = [
    rclpy
    std-srvs

    python3Packages.pyudev
    python3Packages.pygobject-stubs
    python3Packages.pygobject3
  ];
}
