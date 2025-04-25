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
  std-msgs, 
  sensor-msgs, 
  rosidl-default-generators, 
  breakpointHook, 
}:

buildRosPackage {
  pname = "ros-jazzy-gst-msgs";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/BrettRD/ros-gst-bridge";
    sparseCheckout = [
      "gst_msgs"
    ];
    hash = "sha256-2JM2m2PRaK8K3l+P2xWJaa9aOtUmgLk9ot6MY0VdWFQ=";
  };

  sourceRoot = "ros-gst-bridge/gst_msgs";

  buildType = "ament_cmake";
  buildInputs = [ ament-cmake pkg-config breakpointHook ];
  checkInputs = [ ament-cmake-copyright ament-cmake-cppcheck ament-cmake-uncrustify ament-lint-auto ament-lint-common ];
  propagatedBuildInputs = [ rclcpp rclcpp-components std-msgs sensor-msgs rosidl-default-generators ];
  nativeBuildInputs = [ ament-cmake ];
}