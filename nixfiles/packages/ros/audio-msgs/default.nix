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
    sparseCheckout = [
      "audio_msgs"
    ];
    hash = "sha256-5Yvgsqx+1U6jQw60tk4PpAK6Cn/Pz/cNEd90GTRqkZs=";
  };

  sourceRoot = "ros-gst-bridge/audio_msgs";

  buildType = "ament_cmake";
  buildInputs = [ ament-cmake pkg-config breakpointHook ];
  checkInputs = [ ament-cmake-copyright ament-cmake-cppcheck ament-cmake-uncrustify ament-lint-auto ament-lint-common ];
  propagatedBuildInputs = [ rclcpp rclcpp-components std-msgs sensor-msgs rosidl-default-generators ];
  nativeBuildInputs = [ ament-cmake ];
}