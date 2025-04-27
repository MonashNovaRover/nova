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
, gst-bridge
}:

buildRosPackage {
  name = "cameras2";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "cameras2-source";
    path = ../../../cameras2;
    filter = lib.novaSourceFilter [ ] path;
  };

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
    pythonPackages.psutil
    pythonPackages.linuxpy
    
    gst-bridge # ros
  ];

  nativeCheckInputs = [
    pythonPackages.pygobject-stubs
    pythonPackages.types-psutil
  ];

  dontWrapGapps = true;

  preFixup = ''
    wrapGApp "$out/lib/cameras2/camera_streamer_service"
  '';

  doCheck = true;
}
