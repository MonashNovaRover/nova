{ wrapGAppsNoGuiHook, gobject-introspection, gst_all_1, libnice, buildRosPackage
, launch, launch-ros, rclpy, std-srvs, camera-msgs, python3Packages }:

buildRosPackage {
  pname = "ros-cameras2";
  version = "git";
  buildType = "ament_python";

  src = ../../../cameras2;

  nativeBuildInputs =
    [ wrapGAppsNoGuiHook gobject-introspection gst_all_1.gstreamer ];

  buildInputs = [
    gobject-introspection
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
    gst_all_1.gst-plugins-good # V4L2
    gst_all_1.gst-plugins-bad # WebRTC
    gst_all_1.gst-plugins-ugly # H264
    gst_all_1.gst-plugins-rs-webrtc # WebRTC
    libnice # WebRTC
  ];

  propagatedBuildInputs = [
    launch
    launch-ros
    rclpy
    std-srvs
    camera-msgs

    python3Packages.pyudev
    python3Packages.pygobject-stubs
    python3Packages.pygobject3
    python3Packages.gst-python
  ];

  postInstall = ''
    mkdir -p $out/lib/cameras2
    ln -s $out/bin/* $out/lib/cameras2
  '';
}
