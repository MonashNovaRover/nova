{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, rclpy
, std-srvs
, geometry-msgs
, nav-msgs
, sensor-msgs
, eigen
, orocos-kdl
, systemd
, nova-core
}:

buildRosPackage {
  name = "control";
  buildType = "ament_cmake";

  src = lib.cleanNovaSource [
    "!include/jcan_*/libjcan.a"
  ] ../../../control;

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [
    rclcpp
    rclpy
    std-srvs
    geometry-msgs
    nav-msgs
    sensor-msgs
    eigen
    orocos-kdl
    systemd
    nova-core
  ];
}
