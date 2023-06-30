{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, rclpy
, sensor-msgs
, nav-msgs
, opencv
, librealsense2
, nova-core
}:

buildRosPackage {
  name = "autonomous";
  buildType = "ament_cmake";

  src = lib.cleanNovaSource [ ] ../../../autonomous;

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [
    rclcpp
    rclpy
    sensor-msgs
    nav-msgs
    opencv
    librealsense2
    nova-core
  ];
}
