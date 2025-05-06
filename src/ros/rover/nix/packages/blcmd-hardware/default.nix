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
, generate-parameter-library
}:

buildRosPackage {
  name = "blcmd_hardware";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "blcmd-hardware-source";
    path = ../../../hardware_interfaces/blcmd_hardware;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake pkg-config];

  buildInputs = [
    rclcpp
    rclcpp-lifecycle
    hardware-interface
    jcan
    generate-parameter-library
  ];
}
