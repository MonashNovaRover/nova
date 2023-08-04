{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, rclpy
, sensor-msgs
, nav-msgs
, pythonPackages
, nova-core
}:

buildRosPackage {
  name = "autonomous";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "autonomous-source";
    path = ../../../autonomous;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [
    rclcpp
  ];
  propagatedBuildInputs = [
    rclpy
    sensor-msgs
    nav-msgs
    pythonPackages.pyrealsense2
    pythonPackages.opencv4
    pythonPackages.ultralytics
    nova-core
  ];
}
