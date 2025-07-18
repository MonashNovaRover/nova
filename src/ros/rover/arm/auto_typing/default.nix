{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, std-srvs
, geometry-msgs
, sensor-msgs
, python3Packages
, nova-arm-interfaces
, nova-interfaces
, tf2-ros
, tf2-geometry-msgs
, opencv4
, jcan
}:

buildRosPackage {
  name = "auto-typing";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "arm-source";
    path = ../../../arm/auto_typing;
    filter = lib.novaSourceFilter [
    ]
      path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    std-srvs
    geometry-msgs
    sensor-msgs
    tf2-ros
    tf2-geometry-msgs
    nova-arm-interfaces
    nova-interfaces
    opencv4
    jcan
  ];

  propagatedBuildInputs = [
    python3Packages.scipy
    python3Packages.numpy
  ];
}
