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

  src = builtins.path rec {
    name = "control-source";
    path = ../../../control;
    filter = lib.novaSourceFilter [
      "!include/jcan_*/libjcan.a"
    ]
      path;
  };

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
