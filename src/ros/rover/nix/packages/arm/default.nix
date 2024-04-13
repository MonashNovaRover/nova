{ lib
, writeShellApplication
, buildRosPackage
, ament-cmake
, rclcpp
, std-srvs
, geometry-msgs
, nav-msgs
, sensor-msgs
, eigen
, orocos-kdl
, systemd
, pythonPackages
, libcanmd
, nova-arm-interfaces
, nova-input-msgs
, nova-interfaces
}:

buildRosPackage {
  name = "arm";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "arm-source";
    path = ../../../arm/arm;
    filter = lib.novaSourceFilter [
    ]
      path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    std-srvs
    geometry-msgs
    nav-msgs
    sensor-msgs
    eigen
    orocos-kdl
    systemd
    libcanmd
    nova-arm-interfaces
    nova-input-msgs
    nova-interfaces
  ];
}
