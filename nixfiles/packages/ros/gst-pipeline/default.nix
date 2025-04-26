{ buildRosPackage, 
  fetchgit, 
  ament-cmake, 
  ament-cmake-copyright, 
  ament-cmake-cppcheck, 
  ament-cmake-uncrustify, 
  ament-lint-auto, 
  ament-lint-common, 
  pkg-config, 
  rclcpp, 
  rclcpp-components, 
  gst-bridge, 
  image-transport, 
  cv-bridge, 
  class-loader, 
  pluginlib, 
  tinyxml2, 
  std-msgs, 
  std-srvs, 
  gst-msgs, 
  sensor-msgs, 
  audio-msgs, 
  builtin-interfaces, 
  gst_all_1, 
  breakpointHook, 
}:

buildRosPackage {
  pname = "ros-jazzy-gst-pipeline";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/BrettRD/ros-gst-bridge";
    rev = "ac10e220d748bd764b7f67faca941792a8db507a";
    sparseCheckout = [
      "gst_pipeline"
    ];
    hash = "sha256-986roQ+5uy6Kdu8CPtn/HuWkf4VYLL8uaawKh+XIz5c=";
  };

  sourceRoot = "ros-gst-bridge-ac10e22/gst_pipeline";
  cmakeFlags = [ "-DCMAKE_CXX_FLAGS=-I${gst_all_1.gst-plugins-base.dev}/include/gstreamer-1.0"];

  buildType = "ament_cmake";
  buildInputs = [ ament-cmake pkg-config breakpointHook ];
  checkInputs = [ ament-cmake-copyright ament-cmake-cppcheck ament-cmake-uncrustify ament-lint-auto ament-lint-common ];
  propagatedBuildInputs = [ rclcpp rclcpp-components gst-bridge image-transport cv-bridge class-loader pluginlib tinyxml2 std-msgs std-srvs gst-msgs sensor-msgs audio-msgs builtin-interfaces ];
  nativeBuildInputs = [ ament-cmake ];
}