{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, rclcpp-lifecycle
, hardware-interface
, jcan
}:

buildRosPackage {
  name = "rover_hardware";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "rover_hardware-source";
    path = ../../../control;
    filter = lib.novaSourceFilter [ ]
      path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    rclcpp-lifecycle
    hardware-interface
    jcan
  ];
}
