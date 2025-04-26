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
  gst-pipeline, 
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
  breakpointHook, 
}:

buildRosPackage {
  pname = "ros-jazzy-gst-pipeline-plugins";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/BrettRD/ros-gst-bridge";
    sparseCheckout = [
      "gst_pipeline_plugins"
    ];
    hash = "sha256-EapMy+P0uZ7eUIdurhzsQ9qtlJf6t2hNprRJ+TwAg8o=";
  };

  sourceRoot = "ros-gst-bridge/gst_pipeline_plugins";

  buildType = "ament_cmake";
  buildInputs = [ ament-cmake pkg-config breakpointHook ];
  checkInputs = [ ament-cmake-copyright ament-cmake-cppcheck ament-cmake-uncrustify ament-lint-auto ament-lint-common ];
  propagatedBuildInputs = [ rclcpp rclcpp-components gst-pipeline gst-bridge image-transport cv-bridge class-loader pluginlib tinyxml2 std-msgs std-srvs gst-msgs sensor-msgs audio-msgs builtin-interfaces ];
  nativeBuildInputs = [ ament-cmake ];
}