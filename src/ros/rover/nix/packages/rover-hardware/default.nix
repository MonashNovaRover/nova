{ lib
, pkg-config
, buildRosPackage
, ament-cmake
, rclpy
, rclcpp
, rclcpp-lifecycle
, hardware-interface
, pluginlib
, jcan
}:

buildRosPackage {
  name = "rover_hardware";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "rover-hardware-source";
    path = ../../../rover_hardware;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake pkg-config];

  buildInputs = [
    rclcpp
    rclcpp-lifecycle
    hardware-interface
    jcan
  ];
}
