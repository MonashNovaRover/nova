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
  gst-pipeline, 
  pluginlib, 
  gst_all_1, 
  libsoup, 
  libsysprof-capture, # Package 'sysprof-capture-4', required by 'libsoup-2.4', not found
  sqlite, # Package 'sqlite3', required by 'libsoup-2.4', not found
  libpsl, # Package 'libpsl', required by 'libsoup-2.4', not found
  brotli, # Package 'libbrotlidec', required by 'libsoup-2.4', not found
  json-glib, 
  std-msgs, 
  std-srvs, 
  gst-msgs, 
  sensor-msgs, 
  breakpointHook, 
}:

buildRosPackage {
  pname = "ros-jazzy-gst-pipeline-plugins-webrtc";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/BrettRD/ros-gst-bridge";
    rev = "a4d4b74072a087023d98aad866d8f30c48e0814a";
    sparseCheckout = [
      "gst_pipeline_plugins_webrtc"
    ];
    hash = "sha256-wQMkN0n3eT99GKx+7zx5s09eVXKeJXC7skuyqykvHZg=";
  };

  sourceRoot = "ros-gst-bridge-a4d4b74/gst_pipeline_plugins_webrtc";

  buildType = "ament_cmake";
  buildInputs = [ ament-cmake pkg-config breakpointHook ];
  checkInputs = [ ament-cmake-copyright ament-cmake-cppcheck ament-cmake-uncrustify ament-lint-auto ament-lint-common ];
  propagatedBuildInputs = [ rclcpp rclcpp-components gst-pipeline gst-bridge pluginlib gst_all_1.gst-plugins-bad libsoup libsysprof-capture sqlite libpsl brotli json-glib std-msgs std-srvs gst-msgs sensor-msgs ];
  nativeBuildInputs = [ ament-cmake ];
}