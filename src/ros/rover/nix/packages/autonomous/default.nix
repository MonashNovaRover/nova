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

  src = builtins.path rec {
    name = "autonomous-source";
    path = ../../../autonomous;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [
    rclcpp
    opencv
    librealsense2
  ];
  propagatedBuildInputs = [
    rclpy
    sensor-msgs
    nav-msgs
    nova-core
  ];
}
