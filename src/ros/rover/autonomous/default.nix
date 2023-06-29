{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, rclpy
, sensor-msgs
, nav-msgs
, opencv
, librealsense2
, core
}:

buildRosPackage {
  name = "autonomous";
  buildType = "ament_cmake";

  src = lib.cleanNovaSource [ ] ./.;

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [
    rclcpp
    rclpy
    sensor-msgs
    nav-msgs
    opencv
    librealsense2
    core
  ];
}
