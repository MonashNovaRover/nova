{ lib
, writeShellApplication
, buildRosPackage
, ament-cmake
, rclcpp
, rclpy
, std-srvs
, geometry-msgs
, nav-msgs
, trajectory-msgs
, sensor-msgs
, eigen
, orocos-kdl
, systemd
, pythonPackages
, nova-core
, SDL2
, nova-arm-interfaces
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
    trajectory-msgs
    sensor-msgs
    eigen
    orocos-kdl
    systemd
    nova-core
    SDL2
    SDL2.dev
    nova-arm-interfaces
  ];

  propagatedBuildInputs = with pythonPackages; [
    jcan
  ];
}
