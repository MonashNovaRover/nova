{ lib
, buildRosPackage
, wrapGAppsNoGuiHook
, substitute
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
, gst-namedpipe
, glib-networking
, sensor-msgs
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
    (substitute {
      src = ./patches/launch-executable-locations.patch;
      substitutions = [
        "--subst-var-by" "gst_plugins_rs" "${gst_all_1.gst-plugins-rs}"
      ];
    })
    (substitute {
      src = ./patches/launch-patch-ros-streamer.patch;
      substitutions = [
        "--subst-var-by" "gst_plugins_rs" "${gst_all_1.gst-plugins-rs}"
      ];
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
    gst-bridge # ros-gst-bridge/rosimagesrc
    gst-namedpipe # for oakenc
    glib-networking
  ];

  propagatedBuildInputs = [
    launch
    launch-ros
    rclpy
    std-srvs
    sensor-msgs
    nova-camera-msgs

    pythonPackages.pyudev
    pythonPackages.pygobject3
    pythonPackages.gst-python
    pythonPackages.psutil
  ];

  nativeCheckInputs = [
    pythonPackages.pygobject-stubs
    pythonPackages.types-psutil
  ];

  dontWrapGapps = true;

  #export GST_PLUGIN_PATH="${gst-bridge}/lib:$GST_PLUGIN_PATH"
  preFixup = ''
    export GST_PLUGIN_PATH="${gst-bridge}/lib:$GST_PLUGIN_PATH"

    wrapGApp "$out/lib/cameras2/camera_streamer_service"\
      --prefix GST_PLUGIN_PATH : "${gst-bridge}/lib"
    wrapGApp "$out/lib/cameras2/camera_ros_streamer"\
      --prefix GST_PLUGIN_PATH : "${gst-bridge}/lib"
  '';

  doCheck = true;
}
