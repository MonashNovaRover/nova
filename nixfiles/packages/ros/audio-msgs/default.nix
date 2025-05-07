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
  pname = "ros-jazzy-audio-msgs";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/BrettRD/ros-gst-bridge";
    rev = "bce12ef8374e8900d2b56eb0067f23ff6a11a8db";
    sparseCheckout = [
      "audio_msgs"
    ];
    hash = "sha256-3QU3NSWtXiuHbqK+mPcxq7z/t8oxn/J9F7erpAFhbq8=";
  };

  sourceRoot = "ros-gst-bridge-bce12ef/audio_msgs";

  buildType = "ament_cmake";
  buildInputs = [ ament-cmake pkg-config breakpointHook ];
  checkInputs = [ ament-cmake-copyright ament-cmake-cppcheck ament-cmake-uncrustify ament-lint-auto ament-lint-common ];
  propagatedBuildInputs = [ rclcpp rclcpp-components std-msgs sensor-msgs rosidl-default-generators ];
  nativeBuildInputs = [ ament-cmake ];
}