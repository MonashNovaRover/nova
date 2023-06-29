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
, core
}:

buildRosPackage {
  name = "control";
  buildType = "ament_cmake";

  src = lib.cleanNovaSource [
    "!include/jcan_*/libjcan.a"
  ] ./.;

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
    core
  ];
}
