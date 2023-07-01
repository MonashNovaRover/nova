{ lib
, buildRosPackage
, wrapGAppsNoGuiHook
, substituteAll
, gobject-introspection
, gst_all_1
, libnice
, launch
, launch-ros
, rclpy
, std-srvs
, nova-camera-msgs
, pythonPackages
}:

buildRosPackage {
  name = "cameras2";
  buildType = "ament_python";

  src = lib.cleanNovaSource [ ] ../../../cameras2;

  patches = [
    (substituteAll {
      src = ./patches/launch-executable-locations.patch;
      gst_plugins_rs = gst_all_1.gst-plugins-rs;
    })
  ];

  nativeBuildInputs = [
    wrapGAppsNoGuiHook
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
    gst_all_1.gst-plugins-rs # WebRTC
    libnice # WebRTC
  ];

  propagatedBuildInputs = [
    launch
    launch-ros
    rclpy
    std-srvs
    nova-camera-msgs

    pythonPackages.pyudev
    pythonPackages.pygobject3
    pythonPackages.gst-python
  ];
}
