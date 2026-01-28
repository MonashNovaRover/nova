{ lib
, buildRosPackage
#, wrapGAppsNoGuiHook
#, substitute
#, gobject-introspection
#, gst_all_1
#, libnice
, launch
, launch-ros
, rclpy
#, std-srvs
, nova-camera-msgs
, pythonPackages
, gst-bridge
#, glib-networking
#, sensor-msgs
, depthai-core
}:

buildRosPackage {
  name = "oakenc";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "oakenc-source";
    path = ../.;
    filter = lib.novaSourceFilter [ ] path;
  };

  patches = [
  ];

  nativeBuildInputs = [
  ];

  buildInputs = [
  ];

  propagatedBuildInputs = [
    launch
    launch-ros
    rclpy
    #std-srvs
    #sensor-msgs
    nova-camera-msgs

    #pythonPackages.pyudev
    #pythonPackages.pygobject3
    #pythonPackages.gst-python
    #pythonPackages.psutil
    depthai-core
    pythonPackages.numpy
    pythonPackages.pillow
    pythonPackages.opencv4
  ];

  nativeCheckInputs = [
    #pythonPackages.pygobject-stubs
    #pythonPackages.types-psutil
  ];

  #dontWrapGapps = true;

  #export GST_PLUGIN_PATH="${gst-bridge}/lib:$GST_PLUGIN_PATH"
  #preFixup = ''
  #  export GST_PLUGIN_PATH="${gst-bridge}/lib:$GST_PLUGIN_PATH"

  #  wrapGApp "$out/lib/cameras2/camera_streamer_service"\
  #    --prefix GST_PLUGIN_PATH : "${gst-bridge}/lib"
  #  wrapGApp "$out/lib/cameras2/camera_ros_streamer"\
  #    --prefix GST_PLUGIN_PATH : "${gst-bridge}/lib"
  #'';

  doCheck = true;
}
